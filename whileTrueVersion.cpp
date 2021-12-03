// clang++ -std=c++17 -Wall -Werror -Wextra -Wpedantic -g3 -o starter starter.cpp

// Works best in Visual Studio Code if you set:
//   Settings -> Features -> Terminal -> Local Echo Latency Threshold = -1

#include<iostream>
#include<termios.h>
#include<vector>
#include<string>
#include<cmath>
// needed to use sleep function
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

// Types

struct position { unsigned int row; unsigned int col; };
typedef struct position positionstruct;
typedef vector< string > stringvector;

// Constants

// Disable JUST this warning (in case students choose not to use some of these constants)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-const-variable"

const char UP_CHAR    { 'w' };
const char DOWN_CHAR  { 's' };
const char LEFT_CHAR  { 'a' };
const char RIGHT_CHAR { 'd' };
const char QUIT_CHAR  { 'q' };
// https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit
const string ANSI_START { "\033[" };
const string START_COLOUR_PREFIX {"1;"};
const string START_COLOUR_SUFFIX {"m"};
const string STOP_COLOUR  {"\033[0m"};
const unsigned int COLOUR_IGNORE { 0 }; // this is a little dangerous but should work out OK
const unsigned int COLOUR_WHITE  { 37 };
const unsigned int COLOUR_RED    { 31 };
const unsigned int COLOUR_BLUE   { 34 };
const unsigned int COLOUR_BLACK  { 30 };
const unsigned int COLOUR_BRIGHTGREEN  { 92 };

#pragma clang diagnostic pop


const stringvector PLANE {
{"            __/\\__"},
{"           `==/\\==`"},
{" ____________/__\\___________"},
{"/___________________________\\"},
{"  __||__||__/.--.\\__||__||__ "},
{" /__|___|___( >< )___|___|__\\ "},
{"           _/`--`\\_"},
{"          (/------\\)"},
};

const stringvector EMPTYPLANE {
{"                   "},
{"                    "},
{"                             "},
{"                              "},
{"                              "},
{"                               "},
{"                    "},
{"                     "},
};   
}

const stringvector MISSILE { 
{" ,-*"},
{"(_)"},
};

const stringvector EMPTYMISSLE {
{"    "},
{"  "},    
}
// vector <position> missilePositions = {{1, 1}, {1, 1}}; 

vector <unsigned int> missilePositionsY = {1, 1}; 

// Globals testing

struct termios initialTerm; // declaring variable of type "struct termios" named initialTerm

// Utilty Functions

// These two functions are taken from Stack Exchange and are 
// all of the "magic" in this code.
auto SetupScreenAndInput() -> void
{
    struct termios newTerm;
    // Load the current terminal attributes for STDIN and store them in a global
    tcgetattr(fileno(stdin), &initialTerm);
    newTerm = initialTerm;
    // Mask out terminal echo and enable "noncanonical mode"
    // " ... input is available immediately (without the user having to type 
    // a line-delimiter character), no input processing is performed ..."
    newTerm.c_lflag &= ~ICANON;
    newTerm.c_lflag &= ~ECHO;
    // Set the terminal attributes for STDIN immediately
    tcsetattr(fileno(stdin), TCSANOW, &newTerm);
}
auto TeardownScreenAndInput() -> void
{
    // Reset STDIO to its original settings
    tcsetattr(fileno(stdin), TCSANOW, &initialTerm);
}

// Everything from here on is based on ANSI codes
auto ClearScreen() -> void { cout << ANSI_START << "2J" ; }
auto MoveTo( unsigned int x, unsigned int y ) -> void { cout << ANSI_START << x << ";" << y << "H" ; }
auto HideCursor() -> void { cout << ANSI_START << "?25l" ; }
auto ShowCursor() -> void { cout << ANSI_START << "?25h" ; }


auto GetTerminalSize() -> position
{
    // This feels sketchy but is actually about the only way to make this work
    MoveTo(999,999);
    cout << ANSI_START << "6n" ; // ask for Device Status Report 
    string responseString;
    char currentChar { static_cast<char>( getchar() ) };
    while ( currentChar != 'R')
    {
        responseString += currentChar;
        currentChar = getchar();
    }
    // format is ESC[nnn;mmm ... so remove the first 2 characters + split on ; + convert to unsigned int
    // cerr << responseString << endl;
    responseString.erase(0,2);
    // cerr << responseString << endl;
    auto semicolonLocation = responseString.find(";");
    // cerr << "[" << semicolonLocation << "]" << endl;
    auto rowsString { responseString.substr( 0, semicolonLocation ) };
    auto colsString { responseString.substr( ( semicolonLocation + 1 ), responseString.size() ) };
    // cerr << "[" << rowsString << "][" << colsString << "]" << endl;
    auto rows = stoul( rowsString );
    auto cols = stoul( colsString );
    position returnSize { static_cast<unsigned int>(rows), static_cast<unsigned int>(cols) };
    // cerr << "[" << returnSize.row << "," << returnSize.col << "]" << endl;
    return returnSize;
}

// This is pretty sketchy since it's not handling the graphical state very well or flexibly
auto MakeColour( string inputString, 
                 const unsigned int foregroundColour = COLOUR_WHITE,
                 const unsigned int backgroundColour = COLOUR_IGNORE ) -> string
{
    string outputString;
    outputString += ANSI_START;
    outputString += START_COLOUR_PREFIX;
    outputString += to_string( foregroundColour );
    outputString += ";";
    if ( backgroundColour ) { outputString += to_string( ( backgroundColour + 10 ) ); } // Tacky but works
    outputString += START_COLOUR_SUFFIX;
    outputString += inputString;
    outputString += STOP_COLOUR;
    return outputString;
}

// This is super sketchy since it doesn't do (e.g.) background removal
// or allow individual colour control of the output elements.
auto DrawSprite( position targetPosition,
                 stringvector sprite,
                 const unsigned int foregroundColour = COLOUR_WHITE,
                 const unsigned int backgroundColour = COLOUR_IGNORE)
{
    MoveTo( targetPosition.row, targetPosition.col );
    for ( auto currentSpriteRow = 0 ;
                currentSpriteRow < static_cast<int>(sprite.size()) ;
                currentSpriteRow++ )
    {
        cout << MakeColour( sprite[currentSpriteRow], foregroundColour, backgroundColour );
        MoveTo( ( targetPosition.row + ( currentSpriteRow + 1 ) ) , targetPosition.col );
    };
}

auto drawBorder (position screenDimensions) -> void {
    const unsigned int screenHeight = screenDimensions.row;
    const unsigned int screenWidth = screenDimensions.col; 

    for (int i = 0; i < (int) screenHeight; i++) {
        for (int j = 0; j < (int) (screenWidth - 50) / 2; j++ ) {
            MoveTo(i, j); 
            cout << '+'; 
            MoveTo(i, (screenWidth - 50) / 2 + 50 + j); 
            cout << '+'; 
        }
    }
}

// auto collision (unsigned int planeX, unsigned int terminalHeight, position missile) -> bool 
// {

//     // vertical position of missile is less than 5 from bottom
//     if ((int) terminalHeight - missile.yPosition < 8) {
//         // plane is to the left of missile
//         if ( (int) planeX < missile.xPosition && missile.xPosition - planeX <= 30) {
//             return true;  
//         }
//     }
//     return false; 
// }

auto main() -> int
{
    // 0. Set Up the system and get the size of the screen

    SetupScreenAndInput();
    const position TERMINAL_SIZE { GetTerminalSize() };
    if ( ( TERMINAL_SIZE.row < 30 ) or ( TERMINAL_SIZE.col < 50 ) )
    {
        ShowCursor();
        TeardownScreenAndInput();
        cout << endl <<  "Terminal window must be at least 30 by 50 to run this game" << endl;
        return EXIT_FAILURE;
    }

    
    const unsigned int borderWidth = (TERMINAL_SIZE.col - 50) / 2; 
    // const unsigned int borderHeight = TERMINAL_SIZE.row;

    // 1. Initialize State
    position currentPosition { TERMINAL_SIZE.row - 8, borderWidth};
    // GameLoop
    char currentChar {'z'}; // I would rather use a different approach, but this is quick

    int pos1 = 1, pos2 = 1; 
    while (pos1 - pos2 < 30) {
        pos1 = rand()%(50) + borderWidth; 
        pos2 = rand()%(50) + borderWidth; 
    } 

    // 
    while (true)
    {
        // 2. Update State
        if ( currentChar == LEFT_CHAR )  { currentPosition.col = max(  borderWidth,(currentPosition.col - 1) ); }
        if ( currentChar == RIGHT_CHAR ) { currentPosition.col = min( borderWidth + 50 - 30 ,(currentPosition.col + 1) ); }

        // 3. Update Screen

        // 3.A Prepare Screen
    
        HideCursor(); // sometimes the Visual Studio Code terminal seems to forget

        int pos1 = 1, pos2 = 1; 
        while (pos1 - pos2 < 30) {
        pos1 = rand()%(50) + borderWidth; 
        pos2 = rand()%(50) + borderWidth; 
        } 
        


        // 3.B Draw based on state
        // MoveTo( currentPosition.row, currentPosition.col );
        // cout << MakeColour( "><((('>", COLOUR_WHITE, COLOUR_BLUE );
        drawBorder(TERMINAL_SIZE); 
        DrawSprite( {currentPosition.row, currentPosition.col }, PLANE );


        DrawSprite({missilePositionsY[0], (unsigned int) pos1}, MISSILE); 
        DrawSprite({missilePositionsY[1], (unsigned int) pos2}, MISSILE); 
        sleep(20);

        // write collision function here 
        // add two boolean flags to keep track of whether missiles exist on the screen
        // increment vertical position of missile
        // if missiles reach end, reset 


        // MoveTo( 1, 60 );
        // cout << "[" << currentPosition.row << "," << currentPosition.col << "]" << endl;

        // 4. Prepare for the next pass
        currentChar = getchar();

        if (currentChar == QUIT_CHAR ) {
            break; 
        }
        
    }
    // N. Tidy Up and Close Down
    ShowCursor();
    TeardownScreenAndInput();
    cout << endl; // be nice to the next command
    return EXIT_SUCCESS;
}