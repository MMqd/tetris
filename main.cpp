/*
    "tetris" is a simple tetris game, and an example of handling basic terminal input/output on linux, without ncurses.
    Copyright (C) 2021  MMqd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include  <iostream>
#include  <string>
#include  <fstream>

//Used to detect when the program is terminated or the terminal is resized
#include  <signal.h>

//Used to get input without delay, will possibly be replaced by a non - blocking get input command, if it exists
#include  <thread>

//Used for getting user input
#include  <termios.h>
#include  <sys/ioctl.h>
#include  <unistd.h>

//Used for getting user's home directory
#include  <sys/types.h>
#include  <pwd.h>

using namespace std;

bool draw = true;

string drawbuffer = "";
char boardColorIDs[10][20];	//The colors of the game board
char boardColorIDsToDraw[10][20];	//The colors of the game board including the currently unsolidified tetromino

bool tetrominoShapes[5][4][4] = 	//Tetromino shapes (search for "1" to highlight the tetrominos)
		{{{1, 1, 1, 1},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0}},

		 {{1, 1, 1, 0},
		  {1, 0, 0, 0},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0}},

		 {{1, 1, 0, 0},
		  {1, 1, 0, 0},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0}},

		 {{0, 1, 0, 0},
		  {1, 1, 1, 0},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0}},

		 {{1, 1, 0, 0},
		  {0, 1, 1, 0},
		  {0, 0, 0, 0},
		  {0, 0, 0, 0}}};

//Tetromino centers of rotation
bool centersOfRotationX[5] = {1,1,0,1,1};
bool centersOfRotationY[5] = {0,0,0,1,1};

int X = 3;
int Y = 0;
bool currentTetromino[4][4];
bool nextTetromino[4][4];
char currentColor = 1;
char nextColor = 1;
int tetrominoID = 0;
int nextTetrominoID = 0;
int score = 0;
bool spawnNewTetromino = true;	//New tetromino
bool is_GameOver = false;
bool fall = false;
bool is_Paused = false;

//Center of rotation of current tetromino
int centerOfRotationX = 0;
int centerOfRotationY = 0;

int highscore = 0;
string topPadding = "";
string leftPadding = "";

char input_raw[3]{0};	//Input sequences, input_raw[0] stores the character pressed

//Gets the user's local directory to store highscore
const string highscoreFile = string(getpwuid(getuid())->pw_dir) + "/.local/share/Tetris highscore";

//Stores the initial state of the terminal, which is reverted to when the program is closed
struct termios old;

void newTetromino(){
	X = 3;Y = 0;
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			if(nextTetromino[i][j] && boardColorIDs[i + X][j + Y] != 0){is_GameOver = true;break;}
		}
	}
	if(!is_GameOver){
		currentColor = nextColor;
		nextColor = rand() % 7 + 1;
		tetrominoID = nextTetrominoID;
		nextTetrominoID = rand() % 5;
		centerOfRotationX = centersOfRotationX[tetrominoID];
		centerOfRotationY = centersOfRotationY[tetrominoID];
		for(int i = 0; i < 4; i++){
			for(int j = 0; j < 4; j++){
				currentTetromino[i][j] = nextTetromino[i][j];
				nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
			}
		}
	}
	spawnNewTetromino = false;
	draw = true;
}

void Input(){
	while(true){
		//Detects an arrow escape sequence (e.g. "^[[A" which is up)
		//A - (18) up
		//B - (17) down
		//C - (20) left
		//D - (19) right
		//The letters can be viewed by pressing Shift and the desired arrow key in a terminal
		char tmp_input[3]{0};
		read(0, &tmp_input, 3);

		if(tmp_input[0] == '\e'){
			if(tmp_input[1] == '['){
				if(64 < tmp_input[2] && tmp_input[2] < 69){
					tmp_input[0] = tmp_input[2] - 48;
				} else {tmp_input[0] = 0;}//Hides non-input escape sequences
			}
		}
		input_raw[0] = tmp_input[0];
		input_raw[1] = tmp_input[1];
		input_raw[2] = tmp_input[2];
		//Having 3 input buffers is necessary to avoid bugs with arrow keys
	}
}

//Locks the tetromino in place, and scores appropriately
void Solidify(){
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			if(currentTetromino[j][i]){boardColorIDs[j + X][i + Y] = currentColor;}
		}
	}

	char NewColor[10][20];
	int FirstY = 0;
	for(int i = Y; i < 20; i++){
		bool Full = true;
		for(int j = 0; j < 10; j++){if(boardColorIDs[j][i] == 0){Full = false;}}
		
		if(Full){
			for(int j = 0; j < 10; j++){NewColor[j][FirstY] = 0;}
			for(int j = 0; j < 10; j++){
				for(int k = 0; k < i; k++){
					NewColor[j][k + 1] = boardColorIDs[j][k];
					boardColorIDs[j][k] = NewColor[j][k];
				}
				boardColorIDs[j][i] = NewColor[j][i];
			}
			FirstY++;
			score++;
		}
	}
}

bool CheckCollision(int x, int y){
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			int jx = j + x;
			int iy = i + y;
			if(currentTetromino[j][i] && (jx > 9 | jx < 0 | iy >19 | iy < 0 | boardColorIDs[j + x][i + y] != 0)){return true;}
		}
	}
	return false;
}

//Moves the tetromino down every second
void Fall(){
	while(true){
		sleep(1);
		if(!is_GameOver && !is_Paused){fall = true;}
	}
}

//Resize game window
void resizeHandler(int sig){
	system("clear");
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	topPadding = string((w.ws_row - 22)/2,'\n');
	leftPadding = string((w.ws_col - 22)/2,' ');
	draw = true;
}

//Runs when ctrl-C is pressed, returns terminal to normal state and closes the game
void signalHandler(int signum){
	tcsetattr(0, TCSANOW, &old);
	system("clear");
	cout<<"\e[?25h"<<flush;
	exit(signum);  
}

//argc is the size of the argv array
//argv contains each argument, with the 0 being the program name or ./NAME if executed with ./
int main(int argc, const char* argv[]){
	if(argc != 1){
		if(argc > 2){cout<<"Too many arguments"<<endl;return 0;
		} else if(argv[1] == "-h" || argv[1] == "--help"){
			cout<<
			"Simple Tetris Game\n"
			"  Controls:\n"
			"    h/j/l, Arrow Left/Down/Right, n/e/o - Move left/down/right\n"
			"    p,ESC - is_Paused/Unpause\n"
			"    SPACE - Unpause/Restart game after game over"<<endl;
			return 0;
		} else if(argc != 1){cout<<"Unknown argument: " + string(argv[1])<<endl;return 0;}
	}
	srand(time(0));

	ifstream Tmp(highscoreFile);
	Tmp>>highscore;
	Tmp.close();

	//Signals need to be initialized
	signal(SIGWINCH,resizeHandler);
	signal(SIGINT,signalHandler);

	//Gets the terminal size for the first time
	resizeHandler(SIGWINCH);

	nextColor = rand() % 7 + 1;
	currentColor = rand() % 7 + 1;
	tetrominoID = rand() % 5;
	nextTetrominoID = rand() % 5;
	centerOfRotationX = centersOfRotationX[tetrominoID];
	centerOfRotationY = centersOfRotationY[tetrominoID];
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			currentTetromino[i][j] = tetrominoShapes[tetrominoID][j][i];
			nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
		}
	}

	//Used for getting input from terimal without echo, replaces ncurses
	struct termios current;
	tcgetattr(0, &old); /* grab old terminal i/o settings */
	current = old; /* make new settings same as old settings */
	current.c_lflag &= ~ICANON; /* disable buffered i/o */
	current.c_lflag &= ~ECHO; /* set no echo mode */
	tcsetattr(0, TCSANOW, &current);

	thread(Fall).detach();
	thread(Input).detach();
	system("clear");
	while(true){
		char input[3] = {input_raw[0], input_raw[1], input_raw[2]};//Used to avoid race conditions (noticeable with arrow keys)
		input_raw[0] = 0;//Clears input
		if(input[0] == '\e' | input[0] == 'p'){is_Paused = !is_Paused; draw = true;}
		bool S = false;	//Solidifies tetromino when true

		if(!is_GameOver && !is_Paused){
			if(fall){
				if(CheckCollision(X,Y + 1)){
					S = true;spawnNewTetromino = true;
				} else {Y++;}
				draw = true;
				fall = false;
			}

			if(input[0] == 'h' || input[0] == 'n' || input[0] == 20){//Input
				if(!CheckCollision(X - 1,Y)){X--; draw = true;}
			} else if(input[0] == 'j' || input[0] == 'e' || input[0] == 18){
				if(CheckCollision(X, Y + 1)){
					S = true;
					spawnNewTetromino = true;
					draw = true;
				} else {Y++;draw = true;}
			} else if(input[0] == 'k' || input[0] == 'i' || input[0] == 17){
				if(tetrominoID != 2){
					int cX = centerOfRotationX;
					int cY = centerOfRotationY;
					int x = X;
					int y = Y;
					bool TmpT[4][4];
					bool Safe = true;
					cX = 3 - centerOfRotationY; x -= cX - centerOfRotationX;
					cY = centerOfRotationX; y -= cY - centerOfRotationY;
					for(int i = 0; i < 4; i++){
						for(int j = 0; j < 4; j++){
							int jx = j + x;
							int iy = i + y;
							TmpT[j][i] = currentTetromino[i][3 - j];

							if(TmpT[j][i] && (jx > 9 || jx < 0 || iy > 19 || iy < 0 | boardColorIDs[jx][iy] != 0)){Safe = false;}
						}
					}
					if(Safe){
						for(int i = 0; i < 4; i++){
							for(int j = 0; j < 4; j++){currentTetromino[i][j] = TmpT[i][j];}
						}
						centerOfRotationX = cX;centerOfRotationY = cY;X = x;Y = y;
						draw = true;
					}
				}
			} else if(input[0] == 'l' || input[0] == 'o' || input[0] == 19){
				if(!CheckCollision(X + 1, Y)){X++; draw = true;}
			} else if(input[0] == ' '){
				for(int y = Y + 1; y < 21; y++){if(CheckCollision(X, y)){Y = y - 1; break;}} S = true; spawnNewTetromino = true; draw = true;
			//} else if(input[0] == '$'){for(int x = X; x < 10;x++){if(CheckCollision(x, Y)){X = x - 1; break;}} draw = true;	//Experimentation with vim - like commands
			//} else if(input[0] == '^'){for(int x = X; x> - 2;x -  - ){if(CheckCollision(x, Y)){X = x + 1; break;}} draw = true;
			}//Input

			if(S){
				Solidify();
				for(int x = 0; x < 10; x++){
					if(boardColorIDs[x][0] != 0){
						is_GameOver = true;
						break;
					}
				}
				S = false;
			}
			if(spawnNewTetromino && !is_GameOver){newTetromino();}
			if(is_GameOver){
				if(score>highscore){
					ofstream Tmp(highscoreFile);
					highscore = score;
					Tmp<<highscore;
					Tmp.close();
				}
				score = 0;
			}

		} else if(!is_Paused){//Game over
			if(input[0] == ' '){//Input
				X = 3;Y = 0;
				for(int i = 0; i < 20; i++){
					for(int j = 0; j < 10; j++){boardColorIDs[j][i] = 0;}
				}
				nextColor = rand() % 7 + 1;
				currentColor = rand() % 7 + 1;
				tetrominoID = rand() % 5;
				nextTetrominoID = rand() % 5;
				centerOfRotationX = centersOfRotationX[tetrominoID];
				centerOfRotationY = centersOfRotationY[tetrominoID];
				for(int i = 0; i < 4; i++){
					for(int j = 0; j < 4; j++){
						currentTetromino[i][j] = tetrominoShapes[tetrominoID][j][i];
						nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
					}
				}
				S = false;is_GameOver = false;
			}//Input
		}
		if(input[0] == ' '){is_Paused = false; draw = true;}

		if(draw){//Draw
			for(int j = 0; j < 10; j++){
				for(int i = 0; i < 20; i++){boardColorIDsToDraw[j][i] = boardColorIDs[j][i];}
			}
			if(!is_GameOver){
				for(int i = 0; i < 4; i++){
					for(int j = 0; j < 4; j++){
						if(currentTetromino[j][i]){boardColorIDsToDraw[j + X][i + Y] = currentColor;}
					}
				}
			}

			//The \e[1H ovewrites the screen, and returns the cursor to the first row. This "clears" the screen without flicker
			drawbuffer = "\e[1H\e[?25l" + topPadding + leftPadding + "┌────────────────────┬────────────┐\n";
			for(int i = 0; i < 20; i++){
				drawbuffer += leftPadding + "\e[0m│";
				if(is_GameOver || is_Paused){
					switch(i){
						case 9:drawbuffer += "\e[40;37m▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄";break;
						case 10:
							if(is_GameOver){drawbuffer += "\e[47;30m     GAME OVER      ";
							} else if(is_Paused){drawbuffer += "\e[47;30m    GAME PAUSED     ";}
							break;
						case 11:drawbuffer += "\e[40;37m▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀";break;
						default:
							for(int j = 0; j < 10; j++){
								if(boardColorIDsToDraw[j][i] == 0){drawbuffer += "\e[0m  ";}
								else {drawbuffer += "\e[4" + to_string(boardColorIDsToDraw[j][i]) + "m  ";}
							}
							break;
					}
				} else {
					for(int j = 0; j < 10; j++){
						if(boardColorIDsToDraw[j][i] == 0){drawbuffer += "  ";
						} else {drawbuffer += "\e[4" + to_string(boardColorIDsToDraw[j][i]) + "m  \e[0m";}
					}
				}
				drawbuffer += "\e[0m";

				if(i + 1 < 6){
					drawbuffer += "│  ";
					for(int j = 0; j < 4; j++){
						if(nextTetromino[j][i - 1]){drawbuffer += "\e[4" + to_string(nextColor) + "m  \e[0m";
						} else {drawbuffer += "  ";}
					}
					drawbuffer += "\e[0m  │\n";continue;
				}

				switch(i){
					case 0:drawbuffer += "│            │";break;
					case 5:drawbuffer += "├────────────┘";break;
					case 7:drawbuffer += "│ score: " + to_string(score);break;
					case 8:drawbuffer += "│ High score: " + to_string(highscore);break;
					default:drawbuffer += "│";break;
				}
				drawbuffer += '\n';
			}
			drawbuffer += leftPadding + "└────────────────────┘" + topPadding;

			cout<<drawbuffer;
			draw = false;
		}//Draw
		sleep(0.001);//Limits the draw rate to prevent 100% thread usage
	}
}
