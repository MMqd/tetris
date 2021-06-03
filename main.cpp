/*
	This is a simple tetris game, and an example project of how to do basic terminal tasks on linux, without ncurses.
	Copyright (C) 2021  MMqd
	 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <iostream>
#include <string>
#include <fstream>

//Used to detect when the program is terminated or when the terminal is resized
#include <signal.h>

//Used to get input without delay, will be possibly replaced by a delay-less get input command, if I find one
#include <thread>

//Used for getting user input
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

//Used for getting user's home directory
#include <sys/types.h>
#include <pwd.h>

using namespace std;

bool draw=true;

string DrawBuffer="";
char Color[10][20];
char ColorBuffer[10][20];

bool Ts[5][4][4]=	//Tetromino shapes (you can search for 1 to easily highlight the tetrominos)
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

//An array of tetromino centers of rotation
bool CsORX[5]={1,1,0,1,1};
bool CsORY[5]={0,0,0,1,1};

int X=3;
int Y=0;
bool CurrentT[4][4];
bool NextT[4][4];
char CurrentColor=1;
char NextColor=1;
int TID=0;
int NextTID=0;
int Score=0;
bool NT=true;	//New tetromino
bool S=false;	//Solidify
bool GameOver=false;
bool fall=false;
bool Pause=false;

//Current center of rotation
int CORX=0;
int CORY=0;

int HighScore=0;
string After="";
string Before="";

char g[3]{0};	//This array stores input sequences, g[0] stores the character pressed

//Gets the user's local directory to store highscore
const string HighscoreFile=string(getpwuid(getuid())->pw_dir)+"/.local/share/Tetris Highscore";

//Stores the initial state of the terminal, which is reverted to when the program is closed
struct termios old;

void NewT(){
	X=3;Y=0;
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			if(NextT[i][j] && Color[i+X][j+Y]!=0){GameOver=true;break;}
		}
	}
	if(!GameOver){
		CurrentColor=NextColor;
		NextColor=rand()%7+1;
		TID=NextTID;
		NextTID=rand()%5;
		CORX=CsORX[TID];
		CORY=CsORY[TID];
		for(int i=0; i<4; i++){
			for(int j=0; j<4; j++){
				CurrentT[i][j]=NextT[i][j];
				NextT[i][j]=Ts[NextTID][j][i];
			}
		}
	}
	NT=false;draw=true;
}

void Input(){
	while(true){
		read(0,&g,3);
		if(g[0]==27 && g[1]=='[' && 64<g[2] && g[2]<69){g[0]=g[2]-48;}
		//Detects an arrow escape sequence ex: "^[[A" which is up
		//A - up
		//B - down
		//C - left
		//D - right
		//The letters can be viewed by pressing Shift and the desired arrow key
	}
}

void Solidify(){
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			if(CurrentT[j][i]){Color[j+X][i+Y]=CurrentColor;}
		}
	}

	char NewColor[10][20];
	int FirstY=0;
	for(int i=Y; i<20; i++){
		bool Full=true;
		for(int j=0; j<10; j++){if(Color[j][i]==0){Full=false;}}
		
		if(Full){
			for(int j=0; j<10; j++){NewColor[j][FirstY]=0;}
			for(int j=0; j<10; j++){
				for(int k=0; k<i; k++){
					NewColor[j][k+1]=Color[j][k];
					Color[j][k]=NewColor[j][k];
				}
				Color[j][i]=NewColor[j][i];
			}
			FirstY++;
			Score++;
		}
	}
	S=false;
}

bool CheckCollision(int x, int y){
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			int jx=j+x;
			int iy=i+y;
			if(CurrentT[j][i] && (jx>9 | jx<0 | iy>19 | iy<0 | Color[j+x][i+y]!=0)){return true;}
		}
	}
	return false;
}

void Fall(){
	while(true){
		sleep(1);
		if(!GameOver && !Pause){fall=true;}
	}
}

void resizeHandler(int sig){//Resize
	system("clear");
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	After=string((w.ws_row-22)/2,'\n');
	Before=string((w.ws_col-22)/2,' ');
	draw=true;
}

void signalHandler(int signum){//Runs when ctrl-C is pressed and returns terminal to normal state when the game is closed
	tcsetattr(0, TCSANOW, &old);
	system("clear");
	cout<<"\e[?25h"<<flush;
	exit(signum);  
}


//argc is the size of the argv array
//argv contains each argument, with the 0 being the program name or ./NAME if you are executing it with ./
int main(int argc, const char* argv[]){
	if(argc!=1){
		if(argc>2){cout<<"Too many arguments"<<endl;return 0;
		} else if(argv[1]=="-h" || argv[1]=="--help"){
			cout<<
			"Simple Tetris Game\n"
			"  Controls:\n"
			"    h/j/l, Arrow Left/Down/Right, n/e/o - Move left/down/right\n"
			"    p,ESC - Pause/Unpause\n"
			"    SPACE - Unpause/Restart game after game over"<<endl;
			return 0;
		} else if(argc!=1){cout<<"Unknown argument: "+string(argv[1])<<endl;return 0;}
	}
	srand(time(0));

	ifstream Tmp(HighscoreFile);
	Tmp>>HighScore;
	Tmp.close();

	//Signals need to be initialized
	signal(SIGWINCH,resizeHandler);
	signal(SIGINT,signalHandler);

	//Gets the terminal size for the first time
	resizeHandler(SIGWINCH);

	NextColor=rand()%7+1;
	CurrentColor=rand()%7+1;
	TID=rand()%5;
	NextTID=rand()%5;
	CORX=CsORX[TID];
	CORY=CsORY[TID];
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			CurrentT[i][j]=Ts[TID][j][i];
			NextT[i][j]=Ts[NextTID][j][i];
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
		if(g[0]=='\e' | g[0]=='p'){Pause=!Pause;draw=true;}

		if(!GameOver && !Pause){
			if(fall){
				if(CheckCollision(X,Y+1)){
					S=true;NT=true;
				} else {Y++;}
				draw=true;
				fall=false;
			}

			if(g[0]=='h' || g[0]=='n' || g[0]==20){//Input
				if(!CheckCollision(X-1,Y)){X--;draw=true;}
			} else if(g[0]=='j' || g[0]=='e' || g[0]==18){
				if(CheckCollision(X,Y+1)){
					S=true;
					NT=true;
					draw=true;
				} else {Y++;draw=true;}
			} else if(g[0]=='k' || g[0]=='i' || g[0]==17){
				if(TID!=2){
					int cX=CORX;
					int cY=CORY;
					int x=X;
					int y=Y;
					bool TmpT[4][4];
					bool Safe=true;
					cX=3-CORY; x-=cX-CORX;
					cY=CORX; y-=cY-CORY;
					for(int i=0; i<4; i++){
						for(int j=0; j<4; j++){
							int jx=j+x;
							int iy=i+y;
							TmpT[j][i]=CurrentT[i][3-j];

							if(TmpT[j][i] && (jx>9 || jx<0 || iy>19 || iy<0 | Color[jx][iy]!=0)){Safe=false;}
						}
					}
					if(Safe){
						for(int i=0; i<4; i++){
							for(int j=0; j<4; j++){CurrentT[i][j]=TmpT[i][j];}
						}
						CORX=cX;CORY=cY;X=x;Y=y;
						draw=true;
					}
				}
			} else if(g[0]=='l' || g[0]=='o' || g[0]==19){
				if(!CheckCollision(X+1,Y)){X++;draw=true;}
			} else if(g[0]==' '){
				for(int y=Y+1;y<21;y++){if(CheckCollision(X,y)){Y=y-1;break;}}S=true;NT=true;draw=true;
			//} else if(g[0]=='$'){for(int x=X;x<10;x++){if(CheckCollision(x,Y)){X=x-1;break;}}draw=true;	//Experimentation with vim-like commands
			//} else if(g[0]=='^'){for(int x=X;x>-2;x--){if(CheckCollision(x,Y)){X=x+1;break;}}draw=true;
			}//Input

			if(S){
				//GameOver=(Y-CORY<1);// && (X-CORX==3) || (Y-CORY<1);
				Solidify();
				for(int x=0;x<10;x++){
					if(Color[x][0]!=0){
						GameOver=true;
						break;
					}
				}
			}
			if(NT && !GameOver){NewT();}
			if(GameOver){
				if(Score>HighScore){
					ofstream Tmp(HighscoreFile);
					HighScore=Score;
					Tmp<<HighScore;
					Tmp.close();
				}
				Score=0;
			}

		} else if(!Pause){//Game over
			if(g[0]==' '){//Input
				X=3;Y=0;
				for(int i=0; i<20; i++){
					for(int j=0; j<10; j++){Color[j][i]=0;}
				}
				NextColor=rand()%7+1;
				CurrentColor=rand()%7+1;
				TID=rand()%5;
				NextTID=rand()%5;
				CORX=CsORX[TID];
				CORY=CsORY[TID];
				for(int i=0; i<4; i++){
					for(int j=0; j<4; j++){
						CurrentT[i][j]=Ts[TID][j][i];
						NextT[i][j]=Ts[NextTID][j][i];
					}
				}
				S=false;GameOver=false;
			}//Input
		}
		if(g[0]==' '){Pause=false;draw=true;}
		if(g[0]!=0){g[0]=0;}

		if(draw){//Draw
			for(int j=0; j<10; j++){
				for(int i=0; i<20; i++){ColorBuffer[j][i]=Color[j][i];}
			}
			if(!GameOver){
				for(int i=0; i<4; i++){
					for(int j=0; j<4; j++){
						if(CurrentT[j][i]){ColorBuffer[j+X][i+Y]=CurrentColor;}
					}
				}
			}

			//The \e[1H ovewrites the screen, and returns the cursor to the first row. This "clears" the screen without flicker
			DrawBuffer="\e[1H\e[?25l"+After+Before+"┌────────────────────┬────────────┐\n";
			for(int i=0; i<20; i++){
				DrawBuffer+=Before+"\e[0m│";
				if(GameOver || Pause){
					switch(i){
						case 9:DrawBuffer+="\e[40;37m▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄";break;
						case 10:
							if(GameOver){DrawBuffer+="\e[47;30m     GAME OVER      ";
							} else if(Pause){DrawBuffer+="\e[47;30m    GAME PAUSED     ";}
							break;
						case 11:DrawBuffer+="\e[40;37m▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀";break;
						default:
							for(int j=0; j<10; j++){
								if(ColorBuffer[j][i]==0){DrawBuffer+="\e[0m  ";}
								else {DrawBuffer+="\e[4"+to_string(ColorBuffer[j][i])+"m  ";}
							}
							break;
					}
				} else {
					for(int j=0; j<10; j++){
						if(ColorBuffer[j][i]==0){DrawBuffer+="  ";
						} else {DrawBuffer+="\e[4"+to_string(ColorBuffer[j][i])+"m  \e[0m";}
					}
				}
				DrawBuffer+="\e[0m";

				if(i+1<6){
					DrawBuffer+="│  ";
					for(int j=0; j<4; j++){
						if(NextT[j][i-1]){DrawBuffer+="\e[4"+to_string(NextColor)+"m  \e[0m";
						} else {DrawBuffer+="  ";}
					}
					DrawBuffer+="\e[0m  │\n";continue;
				}

				switch(i){
					case 0:DrawBuffer+="│            │";break;
					case 5:DrawBuffer+="├────────────┘";break;
					case 7:DrawBuffer+="│ Score: "+to_string(Score);break;
					case 8:DrawBuffer+="│ High Score: "+to_string(HighScore);break;
					default:DrawBuffer+="│";break;
				}
				DrawBuffer+='\n';
			}
			DrawBuffer+=Before+"└────────────────────┘"+After;

			cout<<DrawBuffer;
			draw=false;
		}//Draw
		sleep(0.001);
	}
}
