// clang++ -std=c++17 -Wall -Werror -Wextra -Wpedantic -g3 -o starter starter.cpp

// Works best in Visual Studio Code if you set:
//   Settings -> Features -> Terminal -> Local Echo Latency Threshold = -1

// TODO: make a map of stringvectors and timers
// add score functionality 
// clear screen upon playing again 
// change speeds
// add colours 



#include<iostream>
#include<termios.h>
#include<vector>
#include<string>
#include<cmath>
#include <map>
#include <chrono>
#include <thread>
#include <time.h>
#include <sys/ioctl.h>
#include <termios.h>
// needed to use sleep function
// #ifdef _WIN32
// #include <Windows.h>
// #else
// #include <unistd.h>
// #endif

using namespace std;

// Types

struct position { unsigned int row; unsigned int col; };
typedef struct position positionstruct;
typedef vector< string > stringvector;

// Constants

// Disable JUST this warning (in case students choose not to use some of these constants)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-const-variable"


const char LEFT_CHAR  { 'a' };
const char RIGHT_CHAR { 'd' };
const char QUIT_CHAR  { 'q' };
const char SPACE_CHAR { ' '}; 
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


// declare game components 
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


const stringvector MISSILE { 
{" ,-*"},
{"(_) "},
};

const stringvector EMPTYMISSILE { 
{"    "},
{"   "},
};

const stringvector BULLET { 
{"||"},
{"/\\"},
};

const stringvector EMPTYBULLET { 
{"  "},
{"  "},
};

const stringvector EXPLOSION {
    {"(\\|/)"},
    {"--0--"},  
    {"(/|\\)"}, 
}; 
const stringvector EMPTYEXPLOSION {
    {"      "},
    {"     "},  
    {"      "},
};


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
        for (int j = 0; j < (int) (screenWidth - 70) / 2; j++ ) {
            MoveTo(i, j); 
            cout << '+'; 
            MoveTo(i, (screenWidth - 70) / 2 + 70 + j); 
            cout << '+'; 
        }
    }
}

// initialize structs to represent game objects
struct missile { position coordinates; int speed;};
struct bullet {position coordinates; int speed; };
typedef vector <bullet> bulletsArray; 
 

map <string, int> difficulties {{"easy", 12}, {"medium" ,9}, {"hard", 6}}; 


// drawing helper functions
auto drawPlane(position planePosition, bool erase) -> void {
    DrawSprite(planePosition, erase ? EMPTYPLANE: PLANE); 
}

auto drawMissile(position bombPosition, bool erase) -> void {
    DrawSprite(bombPosition, erase ? EMPTYMISSILE : MISSILE);  
}


auto drawBullet(position bulletPosition, bool erase) -> void {
    DrawSprite(bulletPosition, erase ? EMPTYBULLET : BULLET); 
}

auto drawBullets (bulletsArray bullets, bool erase) -> void {
    for (bullet bul : bullets) {
        drawBullet(bul.coordinates, erase); 
    }
}

auto drawExplosion(position explosionPosition, bool erase) -> void {
    DrawSprite(explosionPosition, erase? EMPTYEXPLOSION: EXPLOSION); 
}


// Consider this from a Library
bool kbhit()
{
    termios term;
    tcgetattr(0, &term);

    termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}


auto planeBombCollision (unsigned int planeX, unsigned int terminalHeight, missile m) -> bool 
{

    // vertical position of missile is less than 5 from bottom
    if (terminalHeight - m.coordinates.row < 8) {
        // plane is to the left of missile
        if (planeX < m.coordinates.col && m.coordinates.col - planeX <= 30) {
            return true;  
        }
    }
    return false; 
}
auto bulletBombCollision (bulletsArray bullets, missile bomb) -> bool {
    // check positions of each bullet on screen 
    for (bullet bul: bullets) {
        if (bul.coordinates.col >= bomb.coordinates.col && bul.coordinates.col - bomb.coordinates.col < 2) {
            if (bomb.coordinates.row - bul.coordinates.row < 3 && bomb.coordinates.row - bul.coordinates.row > 0) {
                return true; 
            }
        }
    }
    return false; 
}


auto bulletHandle (bulletsArray &bullets, position TERMINAL_SIZE) -> void {
    for (unsigned int i = 0; i < bullets.size(); i++) {
        bullets[i].speed += 2; 
        // bullet speed
        if (bullets[i].speed % 7 == 0) {
            drawBullet({bullets[i].coordinates.row, bullets[i].coordinates.col}, true);
            bullets[i].coordinates.row -= 1;
        }
        if (bullets[i].coordinates.row >= TERMINAL_SIZE.row) {
            bullets.erase(bullets.begin() + i); 
        }
    }
}

auto resetMissile (missile &m, unsigned int borderWidth) -> void{
    m.coordinates.row = 1; 
    m.coordinates.col = rand()%(70) + borderWidth;
    m.speed = 0;  
}

auto displayScore (position TERMINAL_SIZE, int score) {
    const unsigned int scoreMsgRow = 1; 
    const unsigned int scoreMsgCol = TERMINAL_SIZE.col / 2 - 27; 
    MoveTo(scoreMsgRow, scoreMsgCol); 
    cout << "                    "; 
    cout << "Current score: " << score; 

}
// msgID: 1: start2: death, 3: end
auto outputMessage (position TERMINAL_SIZE, int msgID, unsigned int score) -> void {

    const unsigned int errorMsgRow = TERMINAL_SIZE.row / 2 - 5; 
    const unsigned int errorMsgCol = TERMINAL_SIZE.col / 2 - 15; 
    string difficulty; 
    MoveTo(errorMsgRow, errorMsgCol);

    switch (msgID) {
        case 1:
            MoveTo(errorMsgRow, errorMsgCol - 10);
            cout << "---------------------------------------------------------------" << endl;
            MoveTo(errorMsgRow + 2, errorMsgCol + 4);
            cout <<  "Welcome to (insert name), made by team 3" << endl;
            MoveTo(errorMsgRow + 4, errorMsgCol -10);
            cout << "Instructions:" << endl;
            MoveTo(errorMsgRow + 6, errorMsgCol - 10);
            cout << "Avoid the incoming bombs by dodging or shooting them" << endl;
            MoveTo(errorMsgRow + 8, errorMsgCol - 10);
            cout << "Press 'a' to move left, 'd' to move right, spacebar to shoot" << endl;
            MoveTo(errorMsgRow + 10, errorMsgCol - 10);
            cout << "Try to get the highest score possible!" << endl;
            MoveTo(errorMsgRow + 12, errorMsgCol + 9);
            cout << "Press any key to continue" << endl;
            MoveTo(errorMsgRow + 14, errorMsgCol - 10);
            cout << "----------------------------------------------------------------" << endl;
            break;
        case 2:
            cout << "--------------------------------------------" << endl;
            MoveTo(errorMsgRow + 2, errorMsgCol);
            cout <<  "           Select difficulty level:" << endl;
            MoveTo(errorMsgRow + 4, errorMsgCol);
            cout <<  "Type '1' for easy" << score << endl;
            MoveTo(errorMsgRow + 6, errorMsgCol);
            cout << "Type '2' for medium" << endl;
            MoveTo(errorMsgRow + 8, errorMsgCol);
            cout << "Type '3' for hard" << endl;
            MoveTo(errorMsgRow + 10, errorMsgCol);
            cout << "--------------------------------------------" << endl;
            break; 
        case 3:
            cout << "--------------------------------------------" << endl;
            MoveTo(errorMsgRow + 2, errorMsgCol);
            cout <<  "                   You died:(" << endl;
            MoveTo(errorMsgRow + 4, errorMsgCol);
            cout <<  "                   Score: " << score << endl;
            MoveTo(errorMsgRow + 6, errorMsgCol);
            cout << "Press any key to play again, press q to quit" << endl;
            MoveTo(errorMsgRow + 8, errorMsgCol);
            cout << "--------------------------------------------" << endl;
            break; 
        case 4:
            cout << "--------------------------------------------" << endl;
            MoveTo(errorMsgRow + 2, errorMsgCol);
            cout << "           Thank you for playing!" << endl;
            MoveTo(errorMsgRow + 4, errorMsgCol);
            cout <<  "          Your highest score is: " << score << endl;
            MoveTo(errorMsgRow + 6, errorMsgCol);
            cout << "--------------------------------------------" << endl;
            break; 
    }
}


auto main() -> int
{
    using namespace std::this_thread; // sleep_for, sleep_until
    using namespace std::chrono; // nanoseconds, system_clock, seconds

    // 0. Set Up the system and get the size of the screen
    SetupScreenAndInput();
    const position TERMINAL_SIZE { GetTerminalSize() };
    if ( ( TERMINAL_SIZE.row < 30 ) or ( TERMINAL_SIZE.col < 70 ) )
    {
        ShowCursor();
        TeardownScreenAndInput();
        cout << endl <<  "Terminal window must be at least 30 by 70 to run this game" << endl;
        return EXIT_FAILURE;
    }

    const unsigned int borderWidth = (TERMINAL_SIZE.col - 70) / 2; 
    position planePosition { TERMINAL_SIZE.row - 8, borderWidth};
    missile missile1 = {{1, rand()%(70) + borderWidth}, 0}; 
    bool playerAlive = true;
    unsigned int explosionTimer = 0, score = 0, highScore = 0;
    char currentChar;
    string level; 
    string difficulty = "hi"; 
    position explosionCords; 
    bulletsArray bullets;
    // GameLoop
     
    // welcome screen
    while (true) {
        ClearScreen(); 
        outputMessage(TERMINAL_SIZE, 1, score);
        getchar(); 
        break;
    }
    ClearScreen(); 

    // level selection screen
    char c = -1;
    while (c - '0' > 3 || c - '0' < 0) {
        outputMessage(TERMINAL_SIZE, 2, score);
        c = getchar(); 
    }
    // convert char to int value
    int intc = c - '0'; 
    if (intc == 1) {
        difficulty = "easy";
    }
    if (intc == 2) {
        difficulty = "medium";
    }
    if (intc == 3) {
        difficulty = "hard";
    }
    ClearScreen(); 
    
    drawBorder(TERMINAL_SIZE);
 

    while(playerAlive)
    {

        drawPlane( {planePosition.row, planePosition.col }, false);
        drawMissile({missile1.coordinates.row, missile1.coordinates.col}, false);
        displayScore(TERMINAL_SIZE, score); 
        missile1.speed += 1; 
        if (kbhit()) { 
            currentChar = getchar();
            if ( currentChar == LEFT_CHAR )  {
                drawPlane({planePosition.row, planePosition.col }, true); 
                planePosition.col = max(  borderWidth,(planePosition.col - 1) );
                drawPlane( {planePosition.row, planePosition.col }, false);
                }
            if ( currentChar == RIGHT_CHAR ) {
                drawPlane({planePosition.row, planePosition.col }, true);
                planePosition.col = min( borderWidth + 70 - 30 ,(planePosition.col + 1) );
                drawPlane( {planePosition.row, planePosition.col }, false);
                }
            if (currentChar == SPACE_CHAR) {
                int numberOfBullets = (int) bullets.size(); 
                if (numberOfBullets > 0) {
                    // delays bullet to liminate connected bullets
                    if (bullets[numberOfBullets - 1].speed % 5 == 0) {
                        bullets.push_back({{planePosition.row + 2, planePosition.col + 14}, 0});
                    }
                } else {
                    bullets.push_back({{planePosition.row + 2, planePosition.col + 14}, 0});
                }
            }
        }
        // draw all bullets on terminal
        drawBullets(bullets, false);
        HideCursor();

        // delay by 10 ms
        sleep_for(15ms);

        // 10 is the missile speed
        if (missile1.speed % difficulties[difficulty] == 0) {
            drawMissile({missile1.coordinates.row, missile1.coordinates.col}, true);
            missile1.coordinates.row += 2;
        } 

        if (bulletBombCollision(bullets, missile1)) {
            explosionCords = {missile1.coordinates.row, missile1.coordinates.col}; 
            drawMissile(explosionCords, true);
            drawExplosion(explosionCords, false);
            explosionTimer = 1;
            resetMissile(missile1, borderWidth); 
            score += 100; 

        }

        if (planeBombCollision(planePosition.col, TERMINAL_SIZE.row, missile1)) {
            
            if (score > highScore) {
                highScore = score; 
            }

            ClearScreen();
            // 3: output death message
            outputMessage(TERMINAL_SIZE, 3, score);
            sleep_for(1000ms);

            char enteredChar = getchar(); 
            if (enteredChar != 'q') {
                ClearScreen(); 
                drawBorder(TERMINAL_SIZE);
                // make a reset function
                resetMissile(missile1, borderWidth);
                // reset plane to origin 
                planePosition.row = TERMINAL_SIZE.row - 8;
                planePosition.col = borderWidth; 
                // remove existing bullets from vector 
                bullets.clear(); 
                // reset score
                score = 0; 
                // delay by 1 second before starting again 
                sleep_for(1000ms);
            } else {
                // end game loop 
                playerAlive = false; 
                ClearScreen();
                // 4: output end message
                outputMessage(TERMINAL_SIZE, 4, highScore); 
                sleep_for(5000ms); 
                ClearScreen(); 
            }
        }


        // if missile reaches the bottom, reset
        if (missile1.coordinates.row + 2 > TERMINAL_SIZE.row) {
            resetMissile(missile1, borderWidth);
            score += 50; 
        }
        // helper function to reset bullets
        bulletHandle(bullets, TERMINAL_SIZE);

        //
        if (explosionTimer > 0) {
            explosionTimer += 1; 
            if (explosionTimer % 25 == 0) {
                drawExplosion(explosionCords, true);
                explosionTimer = 0; 
            }
        } 

    }
    // N. Tidy Up and Close Down
    ShowCursor();
    TeardownScreenAndInput();
    // cout << "Your Score was " + to_string(speeder) + ". Good Job!" << endl; // be nice to the next command
    return EXIT_SUCCESS;
}


