// ============================================================================
// CFG Generator - Algorithmic Pattern Detection for (unit)^n + tail
// ============================================================================
//
// PURPOSE:
// Given ANY input string, this program algorithmically detects if it follows
// the pattern: (repeating_unit)^n tail  where n > 0
//
// If detected, it generates the CFG:  S -> repeating_unit S | tail
// and shows the language pattern.
//
// If NOT detected, it falls back to the trivial grammar: S -> exact_string
//
// WHY THIS MATTERS:
// In the exam, you're given a SPECIFIC string (like "aabaabaa") and must
// manually find the pattern. This program automates that discovery process,
// which is useful for:
// 1. Verifying your manual analysis
// 2. Understanding the algorithm behind pattern detection
// 3. Testing other strings to see what patterns emerge
//
// THE ALGORITHM:
// --------------
// For each possible unit length from n/2 down to 1:
//   1. Take prefix of that length as candidate unit
//   2. Count consecutive repetitions from start (k times)
//   3. Remaining suffix = tail
//   4. If k > 1 (at least 2 repetitions), we found a pattern!
//   5. Stop at first match (longest unit preferred due to descending loop)
//
// TIME COMPLEXITY: O(n^2) where n = string length
// SPACE COMPLEXITY: O(n) for substrings
//
// LIMITATIONS:
// - Only finds PREFIX-based repeating patterns (not arbitrary subsequences)
// - Prefers longest unit due to loop order (len = n/2 down to 1)
// - Requires k > 1 (at least 2 repetitions), so "abx" won't match "ab" + "x"
//   even though it's technically (ab)^1 x - this is by design for exam patterns
// - Doesn't handle nested patterns (e.g., "ababab" -> could be (ab)^3 or (abab)^1 ab)
// ============================================================================

#include<iostream>
#include<string>
using namespace std;


// ============================================================================
// generatePattern: Core algorithm to detect repeating unit + tail pattern
// ============================================================================
void generatePattern(const string& input)
{
    int n = input.length();
    bool foundPattern = false;

    // Try unit lengths from LONGEST to SHORTEST (n/2 down to 1)
    // WHY n/2? A unit longer than n/2 can't repeat at least twice.
    // WHY descending? We prefer the LONGEST meaningful unit.
    //   Example: "ababab" -> "ab" (len=2, k=3) is better than "a" (len=1, k=6)
    //            "xyzxyz" -> "xyz" (len=3, k=2) is better than "xy" (doesn't repeat)
    for(int len = n/2; len >= 1; len--)
    {
        // Candidate unit = prefix of current length
        string p = input.substr(0, len);

        // Count how many times this unit repeats CONSECUTIVELY from the start
        int k = 0;      // repetition count
        int i = 0;      // current position in input
        while(i + len <= n && input.substr(i, len) == p)
        {
            k++;
            i += len;
        }

        // Pattern found if unit repeats more than once (k > 1)
        // WHY k > 1 and not k > 0?
        // Because k=1 means the unit appears only ONCE, which doesn't
        // establish a "repeating pattern". The exam questions always
        // have at least 2 repetitions of the unit before the tail.
        if(k > 1)
        {
            // The tail is whatever remains after the repetitions
            string q = input.substr(i);

            cout << "Detected pattern: " << p << endl;
            cout << "Detected suffix:  " << (q.empty() ? "Empty (Epsilon)" : q) << endl << endl;

            cout << "Generated CFG:" << endl;
            // If tail is empty, we use epsilon (ε) in the grammar
            cout << "S -> " << p << "S | " << (q.empty() ? "ε" : q) << endl;

            cout << "Generalized Language pattern" << endl;
            // n >= 0 because k>1 means at least 2, but grammar allows n>=0
            // (the base case S -> tail allows n=0)
            cout << "L = (" << p << ")^n " << q << ", where n >= 0" << endl;

            foundPattern = true;
            break;  // Stop at first (longest) match
        }
    }

    // ========================================================================
    // FALLBACK: No repeating pattern found
    // ========================================================================
    // This happens when:
    // - String has no repeated prefix (e.g., "abcdef")
    // - String is too short (< 2 chars)
    // - Only k=1 repetitions found (e.g., "abx" - "ab" appears once then "x")
    if(!foundPattern)
    {
        cout << "No recurring sequential pattern found." << endl;
        cout << "Language pattern:" << endl;
        cout << "L = { " << input << " }" << endl;  // Singleton language
        cout << "Generated CFG:" << endl;
        cout << "S -> " << input << endl;  // Trivial grammar: generates ONLY this string
    }
}


// ============================================================================
// main: Interactive input
// ============================================================================
int main()
{
    string input;
    cout << "Enter the string: ";
    cin >> input;
    generatePattern(input);
    return 0;
}


// ============================================================================
// DETAILED ALGORITHM WALKTHROUGH (for exam understanding):
// ============================================================================
//
// Example: input = "aabaabaa" (length 8)
//
// Iteration 1: len = 4 (n/2)
//   p = "aaba" (first 4 chars)
//   Check: "aaba" == "aaba"? Yes (positions 0-3)
//   Check: "aaba" == "abaa"? No (positions 4-7)
//   k = 1 (only ONE repetition)
//   k > 1? NO -> continue
//
// Iteration 2: len = 3
//   p = "aab" (first 3 chars)
//   Check: "aab" == "aab"? Yes (positions 0-2), k=1, i=3
//   Check: "aab" == "aab"? Yes (positions 3-5), k=2, i=6
//   Check: "aab" == "aa"? No (positions 6-8, only 2 chars left)
//   k = 2, i = 6
//   k > 1? YES!
//   q = input.substr(6) = "aa" (tail)
//   Pattern: "aab", Tail: "aa"
//   CFG: S -> aabS | aa
//   Language: L = (aab)^n aa, n >= 0
//   STOP (break)
//
// Example: input = "abbabbabbc" (length 10)
//
// len=5: p="abbab" -> k=1 (no)
// len=4: p="abba" -> k=1 (no)
// len=3: p="abb" -> "abb"=="abb" (k=1), "abb"=="abb" (k=2), "abb"=="abb" (k=3), i=9
//        "abb"=="c"? No
//        k=3 > 1 YES!
//        q = "c"
//        CFG: S -> abbS | c
//
// Example: input = "abcdef" (no pattern)
//
// len=3: "abc" -> k=1
// len=2: "ab" -> k=1
// len=1: "a" -> k=1
// All k <= 1 -> no pattern found
// Fallback: S -> abcdef
//
// ============================================================================
// KEY INSIGHTS FOR EXAM:
// ============================================================================
//
// 1. The exam strings are DESIGNED to have clear patterns with k >= 2
//
// 2. The loop order (longest to shortest) naturally finds the "most meaningful"
//    unit. But be aware: "aaaaaa" -> len=3 finds "aaa" (k=2), not "a" (k=6).
//    Both are valid! The exam usually expects the shorter unit.
//
// 3. The condition k > 1 is crucial:
//    - "abx" -> "ab" appears once, "x" is NOT a tail in this algorithm's view
//    - But in our manual LAB2 que3.cpp, we allowed n=1 (k=1)!
//    - This algorithm is STRICTER (requires at least 2 repetitions)
//    - For exam: use the manual method from que3.cpp (allows n=1)
//
// 4. The generated CFG uses n >= 0, but our manual analysis used n > 0.
//    Why difference?
//    - Algorithm: S -> unit S | tail  allows n=0 (S -> tail directly)
//    - Manual: We saw n=2 and wrote n>0 because we know there are 2 repetitions
//    - Both are correct grammars; n>0 just excludes the tail-only string
//
// 5. If tail is empty (e.g., "abcabc"), the CFG becomes: S -> abcS | ε
//    This generates (abc)* - zero or more repetitions.
//
// ============================================================================
// HOW TO EXTEND THIS (for deeper understanding):
// ============================================================================
//
// 1. Allow k >= 1 (match que3.cpp behavior):
//    Change: if(k > 1)  ->  if(k >= 1)
//    Then "abx" would give: S -> abS | x
//
// 2. Find ALL possible patterns, not just first:
//    Remove 'break', store all matches, let user choose.
//
// 3. Handle epsilon tail explicitly:
//    If q.empty(), the language includes the empty string when n=0.
//
// 4. Add derivation display (like que3.cpp):
//    Show S => unit S => unit unit S => ... => unit^n tail
//
// 5. Add regex output:
//    Regex: (unit)+tail  (if n>=1) or (unit)*tail (if n>=0)
//
// ============================================================================