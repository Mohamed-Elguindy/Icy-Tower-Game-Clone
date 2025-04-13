#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<windows.h>
#include<time.h>
#include<math.h>
#include"CC212SGL.h"

#pragma(lib,"CC212SGL.lib")

#define wall_width 120
#define lBounds 120
#define rBounds gr.getWindowWidth()-200 
int endGame = -1;
int landflag = 0;
int preY, Y,platspeed;
int character_previous_y;

bool debug = false;
bool _keys[256] = { false };
char score[] = {"999"};
int score_counter = 0;


bool isKeyPressedOnce(int key)
{
	SHORT keyState = GetAsyncKeyState(key);
	bool isCurrentlyPressed = keyState & 0x8000; //if true, then MSB is set
	bool result = false;

	if (isCurrentlyPressed && _keys[key] == false)
	{
		result = true;
		_keys[key] = true;
	}
	else if (!isCurrentlyPressed)
	{
		_keys[key] = false;
	}
	return result;
}


CC212SGL gr;
class GraphicalObject
{
public:

	static enum ANIMATIONS { LEFT, RIGHT, JUMP, CUSTOM1, CUSTOM2, CUSTOM3 };
	static enum BOXS { LEFT_EDGE = 0, RIGHT_EDGE = 1, UPPER_EDGE = 2, LOWER_EDGE = 3 };

	int sprites[20][60]; //Max 60 frames per animation per object
	int totalFramesCount[20] = { -1 }; //Actual number of frames per animation
	float current_frame = 0;
	int frame_dir = 1;

	ANIMATIONS activeAnimation;
	int x = 0, y = 0;
	int box[4] = { 0 };

	void setBoundingBox(int left, int right, int upper, int lower)
	{
		box[0] = left;
		box[1] = right;
		box[2] = upper;
		box[3] = lower;
	}

	int getLeftEdge()
	{
		return x + box[0];
	}

	int getRightEdge()
	{
		return x + box[1];
	}

	int getUpperEdge()
	{
		return y + box[2];
	}

	int getLowerEdge()
	{
		return y + box[3];
	}

	int getCenterX()
	{
		return (getLeftEdge() + getRightEdge()) / 2.0f;
	}

	void loadFrames(const char path[], ANIMATIONS anim, int frames, bool isFullScreen = false)
	{
		for (int i = 0; i < frames; i++)
		{

			char final_loc[100]; //maximum path size is 100 chars
			char num[10];

			itoa(i, num, 10);	//Convert from int to string (ascii)

			strcpy(final_loc, path);	//final_loc = path
			strcat(final_loc, num);	//final_loc = final_loc+num
			strcat(final_loc, ".png");	//final_loc = final_loc+num

			sprites[anim][i] = gr.loadImage(final_loc);
			if (isFullScreen == true)
				gr.resizeImage(sprites[anim][i], gr.getWindowWidth(),
					gr.getWindowHeight());
		}
		totalFramesCount[anim] = frames;	//Save the total number of frames inside the GraphicalObject for later use

	}

	void animate(float speed = 3.0f, bool cycle = true)
	{
		if (cycle)
		{
			//Processing
			if (current_frame <= 0)
				frame_dir = 1;
			else if (current_frame >= totalFramesCount[activeAnimation] - 1)
				frame_dir = 0;

			if (frame_dir == 1)	//Upward direction
				current_frame += 1.0 / speed;
			else if (frame_dir == 0)
				current_frame -= 1.0 / speed;
		}
		else
		{
			frame_dir = 1;
			current_frame += 1.0 / speed;
			if (current_frame >= totalFramesCount[activeAnimation] - 1)
				current_frame = 0.0f;
		}

	}
	void setActiveAnimation(ANIMATIONS active)
	{
		activeAnimation = active;
	}
	virtual void render() //virtual means that the class method can be overriden later on
	{
		gr.drawImage(sprites[activeAnimation][(int)roundf(current_frame)], x, y, gr.generateFromRGB(255, 255, 255));

	}

};

class movingBackground : public GraphicalObject
{
public: 
	void render(int backY) {
		gr.drawImage(sprites[activeAnimation][(int)roundf(current_frame)], x, y-backY, gr.generateFromRGB(255, 255, 255));

	}
};

class Platform : public GraphicalObject
{
public:
	int speed = 1 + rand() % 5;

	void applyPhysics(int q)
	{
		if (preY > Y)
			y -= 0.0225 * q;
		else if (preY == Y) {
			y -= 0.015 * q;
			platspeed = 0.015 * q;
		}
			
	}
	void render(int p) 
	{
		/*
		Custom Code starts here
		*/
		if (debug == true)
		{
			gr.setDrawingColor(COLORS::CYAN);
			int xs, xe, ys, ye;

			xs = x + box[0];
			xe = -box[0] + box[1];
			ys = y + box[2];
			ye = -box[2] + box[3];

			gr.drawRectangle(xs, ys, xe, ye);
		}
		/*
		 End of custom code
		*/
		gr.drawImage(sprites[activeAnimation][(int)roundf(current_frame)], x, y - p, gr.generateFromRGB(255, 255, 255));
	}
};

class MainCharacter : public GraphicalObject
{

public:
	int directionX = 0;
	int directionY = 0;


	//Running Variables
	float acceleration = 8.0f;
	bool isRunning = false;

	//Jumping Variables
	float gravity = 0.98f;
	bool isJumping = false;
	float jumpStrength = 20.0f;
	float yVelo = 0.0f;
	int holdJumpDirection = 0;
	int jumpThrust = 0;

	//Platforming
	Platform* platforms = NULL;
	Platform* boarded = NULL;
	int numPlatforms = 0;

	void move()
	{
		
		// && (character.x-80>wall_width&&character.x<(gr.getWindowWidth()-wall_width-90))

		//x += isRunning ? directionX * acceleration : directionX;
		if(x==rBounds&&directionX<0){
			x += directionX * acceleration;
		}
		else if(x==lBounds&&directionX>0){
			x += directionX * acceleration;
		}
		else if(x>lBounds&&x<rBounds){
			x += directionX * acceleration;
		}


		

		if (boarded)
		{
			if (getCenterX() < boarded->getLeftEdge() || getCenterX() > boarded->getRightEdge())
			{
				startJump(true); //Falling down
			}
		}
		
	}

	void startJump(bool falling = false)
	{
		//To avoid double jump
		if (!isJumping)
		{
			isJumping = true;
			yVelo = jumpStrength * !falling;
			holdJumpDirection = directionX;
			jumpThrust = 2 + isRunning * 4.0f;
			landflag = 0;
			character_previous_y = y;
		}
	}
	void checkPlatform()
	{
		
		for (int i = 0; i < numPlatforms; i++)
		{
			float airDiff = getLowerEdge() - platforms[i].getUpperEdge();
			if (getCenterX() >= platforms[i].getLeftEdge() && getCenterX() <= platforms[i].getRightEdge())
			{
				if (airDiff > -15 && airDiff < 15)
				{
					y = platforms[i].getUpperEdge() - box[3];
					boarded = &platforms[i];
					yVelo = 0;
					landflag = 1;
					isJumping = false;
					//for calculating score
					if (y > preY) {
						//checks if charecter y is above middle for extra score
						if(y<gr.getWindowHeight()/2)
							score_counter += 23;
						else
							score_counter += 10;
					}
					break;
				}
			}
		}
		
		
	}
	void processPhysics()
	{
		//Jump Physics
		if (isJumping)
		{

			y -= yVelo;
			yVelo -= gravity;

			//Character is falling
			if (yVelo < 0)
				checkPlatform();

			//Ground check
			if (y >= gr.getWindowHeight() - 128)
			{
				y = gr.getWindowHeight() - 128;
				yVelo = 0;
				boarded = NULL;
				isJumping = false;
			}
			

			
		}

		//Boarding Physics
		/*if (boarded && directionX == 0)
			x -= boarded->speed;*/

	}
	void render() override
	{
		/*
		Custom Code starts here
		*/
		if (debug == true)
		{
			gr.setDrawingColor(COLORS::CYAN);
			int xs, xe, ys, ye;

			xs = x + box[0];
			xe = -box[0] + box[1];
			ys = y + box[2];
			ye = -box[2] + box[3];

			gr.drawRectangle(xs, ys, xe, ye);
		}
		/*
		 End of custom code
		*/
		GraphicalObject::render();
	}
};

void waitFor(int start, int threshold)
{
	while (true)
	{
		float _t = (clock() / (float)CLOCKS_PER_SEC * 1000.0f - start / (float)CLOCKS_PER_SEC * 1000.0f);
		if (_t >= threshold)
			break;
	}
}


int main() {
	gr.setup();
	gr.setFullScreenMode();
	gr.hideCursor();

	movingBackground background , Rwall,Lwall;
	GraphicalObject END;
	MainCharacter character;
	Platform platforms[20];
	//Platform platforms[20];
	

	system("cls");
	//Loading 
	printf("Loading...");

	background.loadFrames("background", GraphicalObject::CUSTOM1, 1, true);

	Lwall.loadFrames("LeftWall", GraphicalObject::CUSTOM1, 1);
	Rwall.loadFrames("RightWall", GraphicalObject::CUSTOM1, 1);

    character.loadFrames("right", GraphicalObject::RIGHT, 10);
    character.loadFrames("left", GraphicalObject::LEFT, 10);
    character.loadFrames("normal", GraphicalObject::CUSTOM1, 18);
    character.loadFrames("jump", GraphicalObject::JUMP, 3);
    character.loadFrames("rjump", GraphicalObject::CUSTOM3, 3); // right jump
	character.loadFrames("ljump", GraphicalObject::CUSTOM2, 3); // left jump

	END.loadFrames("End", GraphicalObject::CUSTOM1, 1, true);

	for (int i = 0; i < 20; i++)
		platforms[i].loadFrames("log", GraphicalObject::CUSTOM1, 1);

	background.setActiveAnimation(GraphicalObject::CUSTOM1);
	character.setActiveAnimation(GraphicalObject::CUSTOM1);
    Rwall.setActiveAnimation(GraphicalObject::CUSTOM1);
    Lwall.setActiveAnimation(GraphicalObject::CUSTOM1);
	END.setActiveAnimation(GraphicalObject::CUSTOM1);

	for (int i = 0; i < 20; i++)
		platforms[i].setActiveAnimation(GraphicalObject::CUSTOM1);



	Rwall.x=gr.getWindowWidth() - wall_width;
	Lwall.x=0;




	character.x = gr.getWindowWidth()/2;
	character.y = gr.getWindowHeight() - 128;
	Y = character.y;
	character.setBoundingBox(0, 70, 15, 130);
	character.platforms = platforms;
	character.numPlatforms = 20;
	for (int i = 0; i < 20; i++)
	{
		platforms[i].x = (rand() % gr.getWindowWidth())/2+100;
		platforms[i].y = gr.getWindowHeight() - (i * 220);
		platforms[i].setBoundingBox(0, 360, -2, 35);
	
	}
	while (1)
	{
		itoa(score_counter, score, 10);
		int frame_start = clock(); //Time Control Start

		//Keys are helddown
		//Input Handling: Button Presses
		if (character.y< gr.getWindowHeight()-128 && endGame==-1) {
			endGame = 1;
		}
		if (endGame==1&& character.y >= gr.getWindowHeight() - 135) {
			endGame = -1;
			score_counter = 0;
			while (1) {
				gr.beginDraw();
				END.render();
				gr.endDraw();
				if (isKeyPressedOnce('P')|| isKeyPressedOnce('p'))
				{
					return main(); 
				}
				else if (isKeyPressedOnce(VK_ESCAPE)) {
					return 0;
				}

			}
			return 0;
		}
		if (isKeyPressedOnce('D'))
		{
			debug = !debug; //Toggle
		}
		if (isKeyPressedOnce(VK_LEFT) ) // checking x value
		{
			
			character.directionX = -1;	//heading left
			character.setActiveAnimation(GraphicalObject::LEFT);

		}
		if (isKeyPressedOnce(VK_RIGHT))
		{
			character.directionX = 1;	//heading right
			character.setActiveAnimation(GraphicalObject::RIGHT);

		}




		//Keys are held down
		if (GetAsyncKeyState(VK_LEFT) & 0x8000 && !GetAsyncKeyState(VK_RIGHT))
			character.move();
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && !GetAsyncKeyState(VK_LEFT))
			character.move();

		/*if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			character.isRunning = true;
		else
			character.isRunning = false;
        */
		//Nothing is pressed
		if (!GetAsyncKeyState(VK_LEFT) && !GetAsyncKeyState(VK_RIGHT) && !GetAsyncKeyState(VK_SPACE))
		{
			character.setActiveAnimation(GraphicalObject::CUSTOM1);

			character.directionX = 0;
		}

		if (isKeyPressedOnce(VK_SPACE)) {/*
	if(character.directionX == 1){
		character.setActiveAnimation(GraphicalObject::CUSTOM3);
		character.startJump();
		//character.setActiveAnimation(GraphicalObject::RIGHT);
	}
	else if(character.directionX == -1){
		character.setActiveAnimation(GraphicalObject::CUSTOM2);
		character.startJump();
		//character.setActiveAnimation(GraphicalObject::LEFT);
	}
	else {
		character.setActiveAnimation(GraphicalObject::JUMP);
		character.startJump();

	}*/
			if (character.directionX == 0)
				character.setActiveAnimation(GraphicalObject::JUMP);
			character.startJump();
		}

		preY = Y;
		Y = character.y;
		if (Y == preY && character.y<gr.getWindowHeight()-128 )
			character.y -= platspeed;
		
		//Physics
		character.processPhysics();
		for (int i = 0; i < 20; i++) {
			platforms[i].applyPhysics(character.y - gr.getWindowHeight());
			//chatgpt start
			if (platforms[i].getUpperEdge() > gr.getWindowHeight()) { // when platform moves below the screen
				platforms[i].y -= 20 * 220; // dynamic offset
				do {
					platforms[i].x = (rand() % gr.getWindowWidth()) / 2 + 100;
				} while (i > 0 && abs(platforms[i].x - platforms[i - 1].x) < 100); // prevent overlap
				//end of chatgpt
			}
			//if(platforms[i].getUpperEdge() > 1500) {//outside the screen upper edge
				//platforms[i].y -=1500; //regenrate outside the screen upper edge
			   // platforms[i].x = (rand() % gr.getWindowWidth()) / 1.3 + 30;
			//}
		}
			//if(character.y<= gr.getWindowHeight()-128)
			
			

		//Animations
		background.animate();
		Rwall.animate();
		Lwall.animate();
		character.animate(3.0f, false);


		//Rendering
		gr.beginDraw();

		background.render(character.y- gr.getWindowHeight());
		background.render(character.y);
		Rwall.render(character.y - gr.getWindowHeight());
		Rwall.render(character.y);
		Lwall.render(character.y - gr.getWindowHeight());
		Lwall.render(character.y);
		

		
		/*Rwall.render(0);
		Lwall.render(0);*/

		for (int i = 0; i < 20; i++)
			platforms[i].render(0);

		character.render();
		gr.setDrawingColor(SKYBLUE);
		gr.setFontSizeAndBoldness(75, 300);
		gr.drawText(10,gr.getWindowHeight() -1000, score);


		gr.endDraw();
		

		waitFor(frame_start, 1.0 / 30 * 1000);	//Time Control End
	}

	return 0;
}


