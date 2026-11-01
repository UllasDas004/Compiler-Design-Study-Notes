// ============================================================================
// QUESTION: Write a program to implement a lexical analyzer component that
// applies the Maximal Munch (Longest Match) rule to resolve tokenization
// ambiguities.
// ============================================================================
//
// WHAT IS MAXIMAL MUNCH?
// ----------------------
// When the lexer sees certain characters (like '=', '<', '>'), there can be
// ambiguity about how to tokenize them. For example:
//
//   "="  could be:  ASSIGNMENT_OP (single char)
//   "==" could be:  RELATIONAL_OP (double char)
//
// The MAXIMAL MUNCH RULE (also called Longest Match Rule) says:
//   ALWAYS consume the LONGEST valid token starting at the current position.
//
// So when we see '=', we MUST peek at the next character. If it's also '=',
// we consume BOTH as "==" (RELATIONAL_OP). Only if the next character is
// NOT '=', we consume just "=" (ASSIGNMENT_OP).
//
// WHY DOES THIS MATTER?
// - Without maximal munch: "a == b" might tokenize as "a", "=", "=", "b"
//   (two assignment ops) - WRONG!
// - With maximal munch:   "a == b" tokenizes as "a", "==", "b" - CORRECT!
//
// SAME LOGIC APPLIES TO:  ">=" vs ">", "<=" vs "<", "!=" vs "!"
// ============================================================================
//
// TWO-BUFFER INPUT SYSTEM (Review from LAB1):
// -------------------------------------------
// This program still uses the two-buffer technique for efficient file reading.
// Key differences from LAB1's que1.cpp:
//
// | Feature           | LAB1 que1.cpp         | LAB2 que2.cpp              |
// |-------------------|----------------------|---------------------------|
// | Buffer size       | 8 (demo)             | 4096 (production size)    |
// | Sentinel          | '\0' (null char)     | '$' (dollar sign)         |
// | Token types       | Keyword, Identifier  | ALL: kw, id, const, lit, op, delim |
// | Maximal munch     | Not implemented      | YES for ==, !=, <=, >=    |
// | Line tracking     | No                   | Yes                       |
// | Comments          | Not handled          | // comments skipped       |
// | Output format     | Simple               | Formatted table (setw)    |
//
// SENTINEL CHOICE:
// Both '\0' and '$' work as sentinels IF they cannot appear in valid source.
// '\0' is naturally absent from text files. '$' is printable (easier to debug).
// The key rule: SENTINEL MUST BE A CHARACTER THAT CANNOT APPEAR IN SOURCE CODE.
// ============================================================================
//
// ALGORITHM OVERVIEW:
// -------------------
// 1. OPEN file, INIT line=1, PRINT table header
// 2. OUTER LOOP: Read file in BUFFER_SIZE chunks, alternating buffers
//    - Place sentinel '$' after actual data
// 3. INNER LOOP: Scan characters until sentinel '$'
//    Priority order (first match wins):
//    a. '\n'       -> line++, skip
//    b. whitespace -> skip
//    c. "//"       -> skip to end of line
//    d. isalpha/_  -> read word -> keyword/identifier
//    e. isdigit    -> read number -> constant
//    f. '"'        -> read string literal
//    g. '\''       -> read char literal
//    h. op chars   -> check for 2-char ops (maximal munch) -> operator
//    i. delimiters -> single char -> delimiter
//    j. unknown    -> skip
// 4. Toggle buffer flag
// 5. Close file
// ============================================================================

#include <iostream>
#include <fstream>
#include <iomanip>     // for setw() — used to make the table columns aligned
#include <string>
#include <cctype>      // for isalpha(), isdigit(), isspace(), isalnum()

using namespace std;

// BUFFER_SIZE: 4096 bytes (4KB) is standard for production lexers.
// Large buffer = fewer buffer switches = better performance.
// (LAB1 used 8 just to demonstrate switching behavior visually)
const int BUFFER_SIZE = 4096;

// Two buffers + 1 extra byte each for the sentinel ('$')
// We use TWO buffers so one can be scanned while the other is refilled.
// In a real compiler/OS, the refill happens asynchronously (DMA, background thread).
// Here we simulate it by sequentially reading into the inactive buffer.
char buffer1[BUFFER_SIZE + 1];
char buffer2[BUFFER_SIZE + 1];


// ============================================================================
// isKeyword(): Checks if a word is a recognized C keyword.
// ============================================================================
// DESIGN DECISION: Linear search through a small array.
// In a real compiler you'd use unordered_set<string> for O(1) lookup.
// For an exam with ~12 keywords, linear search is perfectly fine and simpler.
// ============================================================================
bool isKeyword(string word)
{
    string keywords[] = {
        "int","float","char","double","if","else",
        "while","for","return","void","break","continue"
    };

    for(string k : keywords)
        if(word == k)
            return true;

    return false;
}


// ============================================================================
// main(): The complete lexical analyzer with Maximal Munch support.
// ============================================================================
int main()
{
    ifstream file("input.txt");

    if(!file)
    {
        cout<<"Cannot open input.txt";
        return 0;
    }

    // 'firstBuffer': tracks which buffer we are currently filling AND scanning.
    // TRUE  -> read into buffer1 next, scan buffer1
    // FALSE -> read into buffer2 next, scan buffer2
    // This alternation means: while scanning buffer1, we'll next fill buffer2,
    // and vice versa. In a real system this enables pipeline parallelism.
    bool firstBuffer = true;

    // 'line': tracks current source line number for the output table.
    // Starts at 1 (line numbers are 1-indexed in all editors/compilers).
    int line = 1;

    // Print the table header using setw() for column alignment.
    // left-justifies the text within each column of specified width.
    // setw(N) sets minimum field width; shorter text is padded with spaces.
    cout<<left<<setw(8)<<"Line"
        <<setw(20)<<"Lexeme"
        <<"Token"<<endl;
    cout<<"---------------------------------------------------"<<endl;


    // ========================================================================
    // OUTER LOOP: Keep reading until the file is exhausted.
    // Each iteration reads one chunk (up to BUFFER_SIZE bytes).
    // ========================================================================
    while(true)
    {
        // Decide which buffer to fill this round.
        // WHY alternate? In a real multi-threaded lexer, one thread scans
        // the active buffer while another thread/DMA fills the inactive one.
        // Here it's sequential, but the structure mirrors the real design.
        if(firstBuffer)
            file.read(buffer1, BUFFER_SIZE);
        else
            file.read(buffer2, BUFFER_SIZE);

        // gcount() = how many bytes were ACTUALLY read from file.
        // This is <= BUFFER_SIZE. At end of file it will be < BUFFER_SIZE.
        int count = file.gcount();

        // If 0 bytes read -> file is completely exhausted. Stop.
        if(count == 0)
            break;

        // Place SENTINEL '$' right after the last real character.
        // WHY not at BUFFER_SIZE? Because count might be less (e.g., 50).
        // Putting '$' at position count means we stop scanning exactly where
        // the real data ends, without reading garbage from uninitialized memory.
        if(firstBuffer)
            buffer1[count] = '$';
        else
            buffer2[count] = '$';

        // 'current' is a pointer to whichever buffer we're scanning this round.
        // Using a pointer means we can write generic code that works for either buffer.
        char *current = firstBuffer ? buffer1 : buffer2;

        // 'i' is our index into 'current'. It moves forward as we consume characters.
        int i = 0;


        // ====================================================================
        // INNER LOOP: Process characters until we hit the sentinel '$'.
        // ====================================================================
        while(current[i] != '$')
        {
            char ch = current[i];

            // ----------------------------------------------------------------
            // NEWLINE: Not a token, but increments the line counter.
            // WHY handle '\n' BEFORE isspace()?
            // Because isspace('\n') is TRUE. If we just "skip whitespace",
            // we'd skip '\n' WITHOUT incrementing the line counter.
            // We MUST check for '\n' explicitly and increment line first.
            // ----------------------------------------------------------------
            if(ch == '\n')
            {
                line++;
                i++;
                continue;
            }

            // ----------------------------------------------------------------
            // WHITESPACE (space, tab, etc.): Not a token, just skip it.
            // isspace() returns true for ' ', '\t', '\v', '\f', '\r'.
            // (Note: '\n' is already handled above)
            // ----------------------------------------------------------------
            if(isspace(ch))
            {
                i++;
                continue;
            }

            // ----------------------------------------------------------------
            // SINGLE-LINE COMMENT "//": Skip everything until end of line.
            // WHY check current[i+1]? Because '/' alone is a valid division
            // operator. We only skip if BOTH characters are '/'.
            // RISK: If '/' is the LAST char in buffer, current[i+1] could be
            // the sentinel '$'. But '$' != '/', so the condition fails safely.
            // ----------------------------------------------------------------
            if(ch == '/' && current[i+1] == '/')
            {
                // Skip chars until newline or sentinel
                while(current[i] != '\n' && current[i] != '$')
                    i++;
                // DON'T increment i here: we want the outer loop
                // to process the '\n' (to count it as a line)
                continue;
            }

            // ----------------------------------------------------------------
            // IDENTIFIER or KEYWORD: Starts with a letter or '_'
            //
            // WHY start with isalpha(ch) || ch=='_' but scan with isalnum?
            // An identifier STARTS with a letter or underscore (not a digit),
            // but CAN CONTAIN digits after the first character.
            // e.g., "x1" is valid identifier, "1x" is not (that's a number then error).
            //
            // MAXIMAL MUNCH here: we keep consuming as long as the character
            // is alphanumeric or '_'. So "integer" is ONE token (Identifier),
            // NOT "int" (Keyword) + "eger" (Identifier).
            // This is crucial: classify AFTER collecting the full word.
            // ----------------------------------------------------------------
            if(isalpha(ch) || ch == '_')
            {
                string word;

                // Collect the entire word (maximal munch for identifiers)
                while(isalnum(current[i]) || current[i] == '_')
                {
                    word += current[i];
                    i++;
                    // NOTE: We do NOT check for '$' here because '$' is not
                    // alphanumeric, so isalnum('$') = false, and the loop stops naturally.
                }

                // Now classify: keyword or identifier?
                cout<<left<<setw(8)<<line
                    <<setw(20)<<word;

                if(isKeyword(word))
                    cout<<"Keyword";
                else
                    cout<<"Identifier";

                cout<<endl;
                continue;
                // WHY continue? Because i is ALREADY past this token.
                // We don't want the loop to do i++ again at the end.
                // Each if-branch takes full control of advancing i.
            }

            // ----------------------------------------------------------------
            // NUMBER (INTEGER or FLOAT CONSTANT):
            // Starts with a digit. We also allow '.' inside (for 3.14).
            //
            // DESIGN NOTE: isdigit(ch) was true -> we entered here.
            // We keep consuming digits AND dots, so "3.14" is one token.
            // A real compiler would be more careful (only one dot allowed,
            // handle scientific notation like 1e-5, etc.) but for an exam
            // this simple approach is sufficient.
            // ----------------------------------------------------------------
            if(isdigit(ch))
            {
                string num;

                while(isdigit(current[i]) || current[i]=='.')
                {
                    num += current[i];
                    i++;
                }

                cout<<left<<setw(8)<<line
                    <<setw(20)<<num
                    <<"Constant"<<endl;

                continue;
            }

            // ----------------------------------------------------------------
            // STRING LITERAL: Starts and ends with double quote "..."
            //
            // ALGORITHM:
            //   1. Consume the opening '"'
            //   2. Keep consuming until we see another '"' or the sentinel
            //   3. Consume the closing '"'
            //
            // WHY stop at '$'? Because the string might not be closed before
            // the buffer ends. Stopping at '$' prevents reading garbage memory.
            // In a real lexer you'd report "unterminated string literal" error.
            // ----------------------------------------------------------------
            if(ch == '"')
            {
                string str;
                str += ch;   // include the opening quote in the lexeme
                i++;

                while(current[i] != '"' && current[i] != '$')
                {
                    str += current[i];
                    i++;
                }

                if(current[i] == '"')   // found the closing quote
                {
                    str += '"';
                    i++;
                }

                cout<<left<<setw(8)<<line
                    <<setw(20)<<str
                    <<"Literal"<<endl;

                continue;
            }

            // ----------------------------------------------------------------
            // CHARACTER LITERAL: Starts and ends with single quote '...'
            // Same logic as string literal but with single quotes.
            // e.g., 'a', '\n', '\t'
            // ----------------------------------------------------------------
            if(ch == '\'')
            {
                string str;
                str += ch;
                i++;

                while(current[i] != '\'' && current[i] != '$')
                {
                    str += current[i];
                    i++;
                }

                if(current[i] == '\'')
                {
                    str += '\'';
                    i++;
                }

                cout<<left<<setw(8)<<line
                    <<setw(20)<<str
                    <<"Literal"<<endl;

                continue;
            }

            // ----------------------------------------------------------------
            // OPERATOR: +, -, *, /, =, <, >, !, %
            //
            // MAXIMAL MUNCH for two-character operators:
            //   "==" "!=" "<=" ">=" should each be ONE token, not two.
            //
            // ALGORITHM:
            //   1. Record the first operator character (op = ch)
            //   2. Check if the NEXT character makes a two-char operator
            //      WHY only for =, <, >, ! and not + or *?
            //      Because "++" and "**" are not in this subset language.
            //      (A full lexer would also handle ++, --, +=, etc. here.)
            //   3. If yes: consume the second char too (maximal munch)
            //   4. If no:  single-char operator
            //
            // NOTE on i++: We consume i++ AFTER the whole if-block,
            // because at entry i still points to ch (the first operator char).
            // The inner i++ (inside the if) consumes the SECOND char.
            // The outer i++ (after the if) consumes the FIRST char.
            // ----------------------------------------------------------------
            if(ch=='+' || ch=='-' || ch=='*' || ch=='/' ||
               ch=='=' || ch=='<' || ch=='>' || ch=='!' ||
               ch=='%')
            {
                string op;
                op += ch;   // start with the first char (i still points here)

                // MAXIMAL MUNCH: Only these four can form two-char operators with '='
                if((ch=='=' || ch=='<' || ch=='>' || ch=='!') &&
                   current[i+1]=='=')
                {
                    op += '=';   // append the second '='
                    i++;         // consume the second character NOW
                }

                cout<<left<<setw(8)<<line
                    <<setw(20)<<op
                    <<"Operator"<<endl;

                i++;   // consume the first character (the one at i when we entered)
                continue;
            }

            // ----------------------------------------------------------------
            // DELIMITER: ;  ,  (  )  {  }  [  ]
            // These are always single-character tokens. No maximal munch needed.
            // WHY are these "delimiters" and not "operators"?
            // By convention: delimiters separate/group language constructs
            // (statements, expressions, blocks) but don't perform computation.
            // They have different roles in grammar productions.
            // ----------------------------------------------------------------
            if(ch==';' || ch==',' || ch=='(' || ch==')' ||
               ch=='{' || ch=='}' || ch=='[' || ch==']')
            {
                string d;
                d += ch;

                cout<<left<<setw(8)<<line
                    <<setw(20)<<d
                    <<"Delimiter"<<endl;

                i++;
                continue;
            }

            // ----------------------------------------------------------------
            // UNKNOWN CHARACTER: anything not matched above (e.g., '@', '#')
            // Just skip it. A production lexer would report a "lexical error"
            // with line/column info, but for exam we silently skip.
            // ----------------------------------------------------------------
            i++;

        } // end inner while (sentinel loop)

        // Toggle buffer for the next chunk read
        firstBuffer = !firstBuffer;

    } // end outer while (file read loop)

    file.close();
    return 0;
}