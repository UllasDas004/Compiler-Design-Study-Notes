// ============================================================================
// QUESTION: Write a Lexical Analyzer (Scanner) in C++ that reads a source
// code file and breaks it down into a stream of lexemes and valid tokens.
// Show the output in tabular form.
// ============================================================================
//
// HOW THIS DIFFERS FROM que1.cpp:
// --------------------------------
// que1.cpp: Focused purely on the TWO-BUFFER MECHANISM. Only classifies
//           keywords vs identifiers. Very minimal classification.
//
// que2.cpp: A COMPLETE Lexical Analyzer. Still uses the two-buffer approach
//           but classifies ALL token types: keywords, identifiers, numbers,
//           string/char literals, operators (with maximal munch), delimiters.
//           Also tracks line numbers and outputs in a formatted table.
//
// SENTINEL DIFFERENCE:
// que1.cpp uses '\0' (null character) as sentinel.
// que2.cpp uses '$'  (dollar sign)   as sentinel.
// Both work the same way logically — it's just a different choice of marker.
// '$' is sometimes preferred because it's a printable character, easier to
// see while debugging. The key rule: pick a character that CANNOT appear in
// your source code naturally.
// ============================================================================

#include <iostream>
#include <fstream>
#include <iomanip>     // for setw() — used to make the table aligned
#include <string>
#include <cctype>      // for isalpha(), isdigit(), isspace(), isalnum()

using namespace std;

// BUFFER SIZE for a full lexer is large (4096 bytes = 4KB) so we rarely
// need to switch buffers. In que1.cpp it was 8 just to demo the switching.
const int BUFFER_SIZE = 4096;

// Two buffers + 1 extra byte each for the sentinel ('$')
char buffer1[BUFFER_SIZE + 1];
char buffer2[BUFFER_SIZE + 1];


// ============================================================================
// isKeyword(): Checks if a word is a recognized C keyword.
//
// WHY does this exist as a separate function (not inline in main)?
// Because we call it in exactly ONE place in the identifier-scanning block.
// Separating it keeps main() readable. Also easy to extend the keyword list.
//
// DESIGN DECISION: Linear search through a small array.
// In a real compiler you'd use an unordered_set for O(1) lookup.
// For an exam with a small keyword list, linear search is fine.
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
// main(): The complete lexical analyzer.
//
// ALGORITHM (high level):
//   1. Open the file.
//   2. OUTER LOOP: Read the file in BUFFER_SIZE chunks, alternating buffers.
//   3. Place sentinel '$' at end of each chunk.
//   4. INNER LOOP: Scan characters until '$':
//       a. '\n'           -> increment line number, continue
//       b. whitespace     -> skip (not a token), continue
//       c. '//' comment   -> skip to end of line, continue
//       d. letter or '_'  -> read whole word -> keyword or identifier
//       e. digit          -> read whole number -> constant
//       f. '"' or '\''    -> read until matching quote -> literal
//       g. operator char  -> check for two-char operators (maximal munch)
//       h. delimiter char -> single-character token
//       i. anything else  -> skip (unknown)
//   5. Toggle buffer flag for next iteration.
//   6. Close file.
// ============================================================================
int main()
{
    ifstream file("input.txt");

    if(!file)
    {
        cout<<"Cannot open input.txt";
        return 0;
    }

    // 'firstBuffer': tracks which buffer we are currently filling.
    // TRUE  -> read into buffer1 next, scan buffer1
    // FALSE -> read into buffer2 next, scan buffer2
    bool firstBuffer = true;

    // 'line': tracks current source line number for the output table.
    // Starts at 1 (line numbers are 1-indexed in editors).
    int line = 1;

    // Print the table header using setw() for column alignment.
    // left-justifies the text within each column of specified width.
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
        // WHY alternate? So we always have one buffer being processed while
        // the other could theoretically be loading (in a real multithreaded
        // system). Here it's sequential, but the structure is the same.
        if(firstBuffer)
            file.read(buffer1, BUFFER_SIZE);
        else
            file.read(buffer2, BUFFER_SIZE);

        // gcount() = how many bytes were ACTUALLY read.
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

        // 'current' is a pointer to whichever buffer we're scanning.
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
            // WHY handle '\n' before isspace()?
            // Because isspace('\n') is true — if we just "skip whitespace",
            // we'd skip '\n' WITHOUT incrementing line. We need '\n' first.
            // ----------------------------------------------------------------
            if(ch == '\n')
            {
                line++;
                i++;
                continue;
            }

            // ----------------------------------------------------------------
            // WHITESPACE (space, tab, etc.): Not a token, just skip it.
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
                continue;   // DON'T increment i here: we want the outer loop
                             // to process the '\n' (to count it as a line)
            }

            // ----------------------------------------------------------------
            // IDENTIFIER or KEYWORD: Starts with a letter or '_'
            //
            // WHY start with isalpha(ch) || ch=='_' but scan with isalnum?
            // An identifier STARTS with a letter or underscore (not a digit),
            // but CAN CONTAIN digits after the first character.
            // e.g., "x1" is valid, "1x" is not (that's a number then error).
            //
            // MAXIMAL MUNCH here: we keep consuming as long as the character
            // is alphanumeric or '_'. So "integer" is one token, NOT "int" + "eger".
            // ----------------------------------------------------------------
            if(isalpha(ch) || ch == '_')
            {
                string word;

                // Collect the entire word (maximal munch)
                while(isalnum(current[i]) || current[i] == '_')
                {
                    word += current[i];
                    i++;
                    // NOTE: We do NOT check for '$' here because '$' is not
                    // alphanumeric, so isalnum('$') = false, and the loop stops.
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
            // A real compiler would be more careful (only one dot allowed, etc.)
            // but for an exam this simple approach is sufficient.
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
            // e.g., 'a', '\n'
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
            //   1. Record the first operator character.
            //   2. Check if the NEXT character makes a two-char operator.
            //      WHY only for =, <, >, ! and not + or *?
            //      Because "++" and "**" are not in this subset language.
            //      (A full lexer would also handle ++ and -- here.)
            //   3. If yes: consume the second char too (maximal munch).
            //   4. If no:  single-char operator.
            //
            // NOTE: We consume i++ AFTER the whole if-block (at line ~206),
            // because at entry i still points to ch (the first operator char).
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
                    i++;         // consume the second character
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
            // Just skip it. A production lexer would report a "lexical error".
            // ----------------------------------------------------------------
            i++;

        } // end inner while (sentinel loop)

        // Toggle buffer for the next chunk read
        firstBuffer = !firstBuffer;

    } // end outer while (file read loop)

    file.close();
    return 0;
}
