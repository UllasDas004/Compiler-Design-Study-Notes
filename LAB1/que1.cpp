// ============================================================================
// QUESTION: Write a program that implements a two-buffer input system to read
// an input source file. Your program must efficiently manage buffer pairs,
// handle lookahead pointers, detect lexemes (like keywords and identifiers),
// and handle end-of-buffer conditions using sentinels.
// ============================================================================
//
// WHY TWO BUFFERS?
// ----------------
// Reading a file character-by-character using fgetc() is very slow because
// each call goes to the OS (a "system call"). Instead, compilers read a whole
// CHUNK at once into a buffer (very fast). But what happens when we reach the
// end of a buffer mid-token? We need the SECOND buffer to be ready to take
// over seamlessly. The two buffers alternate: while we READ from one, we
// RELOAD the other in the background.
//
// MEMORY LAYOUT (BUFFER_SIZE = 8):
// ----------------------------------
//   buffer1: [ h ][ e ][ l ][ l ][ o ][ _ ][ w ][ o ][\0]   <- 8 chars + sentinel
//   buffer2: [ r ][ l ][ d ][ \n][ i ][ n ][ t ][ x ][\0]
//             0    1    2    3    4    5    6    7    8
//             ^--- forwardPtr starts here
//
// The '\0' at position [BUFFER_SIZE] is the SENTINEL. It is a special marker
// that signals "end of this buffer, time to switch to the other one."
// This removes the need to track an index and compare it each iteration.
// ============================================================================

#include<iostream>
#include<fstream>
#include<cctype>
#include<string>

using namespace std;

// WHY BUFFER_SIZE = 8?
// In a real compiler this is 4096 or more for efficiency.
// We use 8 here to clearly demonstrate buffer switching during the demo.
// The +1 is for the sentinel character '\0' at the end.
const int BUFFER_SIZE = 8;

// Two global buffers. We declare them globally so loadBuffer() and getChar()
// can both access them without passing pointers around everywhere.
char buffer1[BUFFER_SIZE + 1];   // holds first chunk of file + sentinel
char buffer2[BUFFER_SIZE + 1];   // holds second chunk of file + sentinel

// Global file stream. Also global so all functions can read from it.
ifstream fin;

// forwardPtr: This is the SCANNER pointer. It always points to the next
// character to be read. It starts at buffer1[0] and moves forward one
// character at a time as we call getChar().
char *forwardPtr;

// lexemeBegin: This marks the START of the current token being built.
// In this simplified version we don't use it for actual substring extraction
// (we use the 'lexeme' string in main instead), but it represents the concept
// of the "begin marker" in the classic two-buffer design.
char *lexemeBegin;

// usingFirst: tracks which buffer forwardPtr is currently inside.
// TRUE  -> forwardPtr is inside buffer1
// FALSE -> forwardPtr is inside buffer2
bool usingFirst = true;

// ============================================================================
// loadBuffer(): Reads the next BUFFER_SIZE characters from the file into buf.
//
// WHY do we place '\0' at position n (not BUFFER_SIZE)?
// Because the file might have fewer than BUFFER_SIZE characters left.
// fin.gcount() tells us how many were actually read. We place '\0' right
// after the last real character so the sentinel is in the correct position.
// If the file is exhausted, n=0 and buffer[0]='\0', which immediately signals
// "end of input" to getChar().
// ============================================================================
void loadBuffer(char buffer[]){
    fin.read(buffer, BUFFER_SIZE);    // read up to BUFFER_SIZE bytes from file
    int n = fin.gcount();             // actual number of bytes read (may be < BUFFER_SIZE)
    buffer[n] = '\0';                 // place sentinel right after the real data
}


// ============================================================================
// getChar(): Returns the next character from the file stream via the buffers.
//
// THE CORE OF THE TWO-BUFFER (PING-PONG) SYSTEM:
//
//  1. Read the character at *forwardPtr.
//  2. If it's NOT '\0': advance forwardPtr, return the character. Done.
//  3. If it IS '\0' (sentinel hit):
//       - Switch forwardPtr to the OTHER buffer IMMEDIATELY (it was already
//         pre-loaded and waiting, so there is ZERO delay).
//       - Refill the CURRENT (consumed) buffer with the next chunk from disk.
//         * In a production compiler / OS: this refill happens asynchronously
//           in the background (DMA / worker thread) while the CPU scans the other buffer.
//         * In our single-threaded code: we call loadBuffer() right here to simulate
//           the background reload so the buffer is ready before we switch back to it.
//       - Toggle usingFirst flag.
//       - Read the first character of the newly active buffer.
//  4. Advance forwardPtr and return the character.
//
// NOTE: If the file is empty (or the other buffer also has no data), the
// sentinel in the new buffer will also be '\0' at position 0, so the caller
// (main) will see '\0' and know to stop.
// ============================================================================
char getChar(){
    char ch = *forwardPtr;   // peek at current position

    if(ch == '\0'){           // SENTINEL HIT -> end of current buffer
        if(usingFirst){
            // We just finished buffer1!
            // 1. Switch to buffer2 immediately (it is ALREADY loaded and waiting)
            forwardPtr = buffer2;
            usingFirst = false;
            // 2. Refill buffer1 with the NEXT chunk so it's ready when buffer2 finishes
            loadBuffer(buffer1);
        } else {
            // We just finished buffer2!
            // 1. Switch to buffer1 immediately (it is ALREADY loaded and waiting)
            forwardPtr = buffer1;
            usingFirst = true;
            // 2. Refill buffer2 with the NEXT chunk so it's ready when buffer1 finishes
            loadBuffer(buffer2);
        }

        // After switching, read the first character of the newly active buffer
        ch = *forwardPtr;
    }

    forwardPtr++;    // advance the pointer AFTER reading (post-increment)
    return ch;       // return the character we read
}


// ============================================================================
// isKeyword(): Checks if a given string is a C keyword.
//
// WHY check AFTER collecting the whole word?
// Because we can't tell if "int" is a keyword until we've read all three
// characters. A greedy (maximal munch) lexer always reads the longest possible
// sequence of valid identifier characters first, THEN classifies.
// This way "integer" is correctly classified as an IDENTIFIER, not
// keyword "int" followed by identifier "eger".
// ============================================================================
bool iskeyWord(string s){
    // A hardcoded list of C keywords we recognize.
    // In a real compiler this might be a hash map for O(1) lookup.
    string keyWords[] = {"int","float","char","if","else","while","for","return","void","double"};

    for(string str: keyWords){
        if(str == s){
            return true;   // found a match -> it IS a keyword
        }
    }

    return false;   // no match found -> it's an identifier
}


// ============================================================================
// main(): Orchestrates the two-buffer read loop.
// ============================================================================
int main(){
    fin.open("input.txt");

    if(!fin){
        cout<<"File not found"<<endl;
        return 0;
    }

    // INITIAL BUFFER LOAD:
    // We load BOTH buffers at the start.
    // WHY load buffer2 now if we haven't even read buffer1 yet?
    // In the original two-buffer design, both buffers are filled upfront
    // so that when forwardPtr hits the sentinel in buffer1, buffer2 is
    // already ready (no wait). Here, loadBuffer() reads sequentially from
    // the file, so calling it twice fills buffer1 then buffer2 in order.
    loadBuffer(buffer1);
    loadBuffer(buffer2);

    // Set both pointers to the START of buffer1.
    // lexemeBegin: marks the start of the token we're building.
    // forwardPtr:  the scanner; this is the one that actually moves forward.
    forwardPtr  = buffer1;
    lexemeBegin = buffer1;

    // 'lexeme' accumulates characters of the current token being scanned.
    // It's cleared every time a token boundary is found (non-alnum char).
    string lexeme = "";


    // ========================================================================
    // MAIN SCAN LOOP
    // ========================================================================
    while(true){
        char c = getChar();   // get next character (handles buffer switching)

        // TERMINATION CONDITION:
        // If getChar() returns '\0', we've exhausted both buffers (file ended).
        // This happens when loadBuffer() reads 0 bytes and sets buffer[0] = '\0'.
        if(c == '\0'){
            break;
        }

        if(isalnum(c) || c == '_'){
            // This character is part of an identifier or keyword.
            // WHY isalnum? Identifiers can contain letters (isalpha) AND digits (isdigit).
            // WHY c == '_'? C identifiers can also start with or contain underscores.
            // We do NOT consume '+', '-', spaces, etc. here — they are token boundaries.
            lexeme += c;   // accumulate into the current token string

        } else {
            // We hit a NON-identifier character (space, operator, newline, etc.).
            // This means the current lexeme (if any) has ENDED.
            if(!lexeme.empty()){
                // We have a complete word -> classify it.
                // WHY check empty? Because two consecutive non-alnum chars (e.g., "+=")
                // would otherwise try to classify an empty string as a token.
                if(iskeyWord(lexeme)){
                    cout << lexeme << "-> Keyword\n";
                } else {
                    cout << lexeme << "-> Identifier\n";
                }
                lexeme.clear();   // reset for the next token
            }
            // The non-alnum character itself (e.g., '+', ';') is ignored in this
            // simple version. A full lexer would classify operators/delimiters too.
        }
    }

    // ========================================================================
    // HANDLE THE LAST TOKEN (no trailing whitespace case)
    // WHY is this needed?
    // If the file ends without any whitespace/newline after the last word,
    // e.g., "int main" with no trailing newline, the loop exits via '\0'
    // break WITHOUT ever hitting the else-branch that classifies the last lexeme.
    // So we must check and print lexeme one final time after the loop.
    // ========================================================================
    if(!lexeme.empty()){
        if(iskeyWord(lexeme)){
            cout << lexeme << "-> Keyword\n";
        } else {
            cout << lexeme << "-> Identifier\n";
        }
    }

    fin.close();
    return 0;
}
