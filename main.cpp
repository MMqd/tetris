#include <iostream>
#include <string>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <signal.h>

using namespace std;

bool draw=true;

string DrawBuffer="";
char Getch;
char Color[10][20];
char ColorBuffer[10][20];
bool Ts[5][4][4]=
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
bool NT=true;
bool S=false;
bool GameOver=false;
bool fall=false;
bool Pause=false;
int CORX=0;
int CORY=0;
int HighScore=0;
string After="";
string Before="";

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
		//Getch
		Getch=getchar();
		//Getch
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

void resizeHandler(int sig){
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	After=string((w.ws_row-22)/2,'\n');
	Before=string((w.ws_col-22)/2,' ');
	draw=true;
}

void signalHandler(int signum) {
	tcsetattr(0, TCSANOW, &old);
	system("clear");
	cout<<"\e[?25h"<<flush;
	exit(signum);  
}

int main(int argc, const char* argv[]){
	if(argc!=1){
		if(argc>2){cout<<"Too many arguments"<<endl;return 0;
		} else if(argv[1]=="-h" || argv[1]=="--help"){
			cout<<
				"Simple Tetris Game\n"
				"  Controls:\n"
				"    n - Move left\n"
				"    e - Move down\n"
				"    o - Move right\n"
				"    p,ESC - Pause/Unpause\n"
				"    SPACE - Unpause/Restart game after game over"<<endl;
			return 0;
		} else if(argc!=1){cout<<"Unknown argument: "+string(argv[1])<<endl;return 0;
		}
	}
	srand(time(0));

	ifstream Tmp("HighScore");
	Tmp>>HighScore;
	Tmp.close();

	signal(SIGWINCH,resizeHandler);
	signal(SIGINT,signalHandler);
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
		if(Getch=='\e' | Getch=='p'){Pause=!Pause;draw=true;}

		if(!GameOver && !Pause){
			if(fall){
				if(CheckCollision(X,Y+1)){
					S=true; NT=true;
				} else {Y++;}
				draw=true;
				fall=false;
			}
			switch(Getch){//Input
				case 'n':if(!CheckCollision(X-1,Y)){X--;draw=true;}break;
				case 'e':
					if(CheckCollision(X,Y+1)){
						S=true;
						NT=true;
						draw=true;
					} else {Y++;draw=true;}
					break;
				case 'i':
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

								if(TmpT[j][i] && (jx>9 | jx<0 | iy>19 | iy<0 | Color[jx][iy]!=0)){Safe=false;}
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
					break;
				case 'o':if(!CheckCollision(X+1,Y)){X++;draw=true;}break;
				/*case '$':for(int x=X;x<10;x++){if(CheckCollision(x,Y)){X=x-1;break;}}draw=true;break;
				case '^':for(int x=X;x>-2;x--){if(CheckCollision(x,Y)){X=x+1;break;}}draw=true;break;*/
				case ' ':for(int y=Y+1;y<21;y++){if(CheckCollision(X,y)){Y=y-1;break;}}S=true;NT=true;draw=true;break;

			}//Input

			if(S){
				//GameOver=(Y-CORY<1);// && (X-CORX==3) | (Y-CORY<1);
				Solidify();
				for(int x=0;x<10;x++){
					if(Color[x][0]!=0){
						GameOver=true;
						if(Score>HighScore){
							ofstream Tmp("HighScore");
							HighScore=Score;
							Tmp<<HighScore;
							Tmp.close();
						}
						Score=0;
						break;
					}
				}
			}
			if(NT && !GameOver){NewT();}

		} else if(!Pause){//Game over
			if(Getch==' '){//Input
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
				draw=true;S=false;GameOver=false;
			}//Input
		}
		if(Getch==' '){Pause=false;draw=true;}
		if(Getch){Getch=0;}

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

			DrawBuffer="\e[1H\e[?25l"+After+Before+"┌────────────────────┬────────────┐\n";
			for(int i=0; i<20; i++){
				DrawBuffer+=Before+"\e[0m│";
				if(GameOver | Pause){
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
