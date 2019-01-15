#include <windows.h>
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <iostream>

#include "displaybuffer.h"
#include "graphTree.h"
#include "toolTip.h"

#define ttipHeight 25
#define SELBOUND 0.05

bool gKeyboard[256];
float cameraX, cameraY, cameraZ;
int screenWidth = 1024, screenHeight = 768;

displaybuffer undoBuffer;
std::list<graphTree*>::iterator displayItr;
toolTip ttip;
bool premiseMode = true, takeInTxt = false;
graphNode* nNode = 0;
int mainWindow, tooltipWindow;
char inputText[128];
unsigned int inTxtPos;

std::pair<selectObj*,graphNode*> alterObj;

//Display functions
void init();
void render();
void renderTooltip();
void resize(int width, int height);

void makeInputWindow(char* lbl);
void destroyInputWindow();

//Keypress functions
void Keyboard(unsigned char key, int x, int y);
void KeyboardUp(unsigned char key, int x, int y)		{ gKeyboard[key] = false; };
void KeyboardSpec(int key, int x, int y);
void resetInput()	{ inTxtPos = 0; for(unsigned int i = 0; i < 128; i++) inputText[i] = 0; };
void keyboardTTip(unsigned char key, int a, int b);

//Mouse button click
void Mouse(int button, int state, int x, int y);

//Mouse motion functions
void MouseActMove(int x, int y);
void MousePassMove(int x, int y);

//Support functions
std::pair<float,float> getLoc(int x, int y);

void animate(int value);

void undo();
void redo();
void reset();

//Inference functions
bool verifyStep(std::string rule);
bool verifyRec(std::string rule, std::list<graphNode*> used, std::list<std::pair<graphNode*,bool> >::iterator itr);
void applyStep(std::string rule);

bool contains(std::list<graphNode*> lst, graphNode* obj);

int main(int argc, char** argv)
{
	for(int i=0; i<256; ++i)
		gKeyboard[i] = false;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition(100,100);
	mainWindow = glutCreateWindow("Peirce -- Existential Graphs");
		init();
		glutDisplayFunc(render);
		glutReshapeFunc(resize);

		//Input function associations
		glutKeyboardFunc(Keyboard);
		glutKeyboardUpFunc(KeyboardUp);
		glutSpecialFunc(KeyboardSpec);
		glutMouseFunc(Mouse);
		glutMotionFunc(MouseActMove);
		glutPassiveMotionFunc(MousePassMove);

	tooltipWindow = glutCreateSubWindow(mainWindow,0,screenHeight-ttipHeight,screenWidth,ttipHeight);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45,(GLdouble)screenWidth/ttipHeight, 2,10);
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.2,0.2,0.2,0.5);
		glutDisplayFunc(renderTooltip);
		glutKeyboardFunc(Keyboard);
		glutKeyboardUpFunc(KeyboardUp);


	//Load configuration file to get Undo levels, selection colors, etc.
	//Code goes here
	undoBuffer.init(5);
	displayItr = undoBuffer.begin();

	alterObj.first = new selectObj();
	alterObj.second = 0;

	//Create undo buffer as a displaybuffer object.
	//Get iterator for it.

	glutTimerFunc(10,animate,0);
	glutMainLoop();
	return 0;
}

void init()
{
	//glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,(GLdouble)screenWidth/(GLdouble)screenHeight, 1,20);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(1.0,1.0,1.0,1.0);

	cameraX = 0;
	cameraY = 0;
	cameraZ = 15;
	ttip.setText("Premise Drawing Mode.");
	ttip.setColor(0.9,0.1,0.7);
}

void render()
{
	glutSetWindow(mainWindow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(cameraX,cameraY,cameraZ,cameraX,cameraY,0,0,1,0);

	if (nNode != 0) nNode->draw();
	if (alterObj.second != 0 && alterObj.second != alterObj.first->original) alterObj.second->draw();
	(*displayItr)->draw();

	if (takeInTxt)
	{
		glColor3f(0.5,0.5,0.5);
		glRasterPos2f(nNode->x1 - 0.1,nNode->y1 - 0.1);
		for (unsigned int i = 0; i < inTxtPos; i++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, inputText[i]);
	}

	glutSwapBuffers();
}

void renderTooltip()
{
	glutSetWindow(tooltipWindow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(0,0,7,0,0,0,0,1,0);

	ttip.draw(screenWidth,ttipHeight);
	glutSwapBuffers();
}

void resize(int width, int height)
{
	//Resize main window
	glutSetWindow(mainWindow);
	screenWidth = width;
	screenHeight = height;
	glViewport(0,0,(GLsizei)width,(GLsizei)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,(GLdouble)width/(GLdouble)height, 1,20);
	glMatrixMode(GL_MODELVIEW);

	//Resize tooltip subwindow
	glutSetWindow(tooltipWindow);
	glutPositionWindow(0,height-ttipHeight);
	glutReshapeWindow(width,ttipHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,(GLdouble)width/ttipHeight, 2,10);
	glMatrixMode(GL_MODELVIEW);
}

//Keyboard key is pressed
void Keyboard(unsigned char key, int x, int y)
{
	gKeyboard[key] = true;
	int mod = glutGetModifiers();

	if (takeInTxt)
	{
		if (inTxtPos >= 127)
			return;
		if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'))
		{
			inputText[inTxtPos] = key;
			++inTxtPos;
		}
		else if (key == 8) //Backspace
		{
			if (inTxtPos > 0)
				--inTxtPos;
		}
		else if (key == 13) //Enter
		{
			nNode->setLabel(inputText);
			nNode->lblLen = inTxtPos;

			graphNode* parent = (*displayItr)->getLowestSubtree(nNode->x1, nNode->y1);
			(*displayItr)->add(parent,nNode);
			nNode = 0;
			takeInTxt = false;
			resetInput();
			ttip.setText("Premise Drawing Mode.");
			glutSetWindow(tooltipWindow);

			(*displayItr)->debugPrint();
		}
		else if (key == 27) //Esc
		{
			resetInput();
			delete nNode;
			nNode = 0;
			takeInTxt = false;
			ttip.setText("Premise Drawing Mode.");
			glutSetWindow(tooltipWindow);
			glutPostRedisplay();
		}
	}

	if (key == 'x' && mod == GLUT_ACTIVE_ALT) exit(0);
	if (key == 'n' && mod == GLUT_ACTIVE_ALT) reset();
	if (key == 'p' && mod == GLUT_ACTIVE_ALT) (*displayItr)->debugPrint();

	if (premiseMode)
	{
		if (key == 'r' && mod == GLUT_ACTIVE_ALT)
		{
			premiseMode = false;
			ttip.setText("Inference Mode.");
			glutSetWindow(tooltipWindow);
			glutPostRedisplay();
		}
		if (key == 127) //Delete
		{
			(*displayItr)->deleteSelections();
			(*displayItr)->debugPrint();
		}
	}
	else
	{
		if (key == 'z' && mod == GLUT_ACTIVE_ALT) undo();
		if (key == 'y' && mod == GLUT_ACTIVE_ALT) redo();
		if (key == '1' && mod == GLUT_ACTIVE_ALT)
		{
			if (verifyStep("DoubleCutRemove"))
				applyStep("DoubleCutRemove");
		}
		if (key == '2' && mod == GLUT_ACTIVE_ALT)
		{
			if (verifyStep("Deiteration"))
				applyStep("Deiteration");
		}
		if (key == '3' && mod == GLUT_ACTIVE_ALT)
		{
			if (verifyStep("Erasure"))
				applyStep("Erasure");
		}
		if (key == 127)	//Delete
		{
			if (verifyStep("DoubleCutRemove"))
			{
				applyStep("DoubleCutRemove");
			}
			else if (verifyStep("Deiteration"))
			{
				applyStep("Deiteration");
			}
			else if (verifyStep("Erasure"))
			{
				applyStep("Erasure");
			}
			else
			{
			}
		}
	}
}

void KeyboardSpec(int key, int x, int y)
{
}

//Mouse button is pressed or released
//This function is so horribly written as things got
//tacked on whereever as functionality was added.
//I'm sorry :(
void Mouse(int button, int state, int x, int y)
{
	int mod = glutGetModifiers();

	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON && alterObj.second != 0)
	{
		std::cout << "Ending alteration operation.\n";
		if (premiseMode == false && alterObj.first->type == 3)
		{
			//Copying an object in Inference Mode can only
			//be done with Iteration (to the same subtree)
			//or Insertion (to an odd level anywhere).
			if (verifyStep("Iteration"))
				applyStep("Iteration");
			else if (verifyStep("InsertionC"))
				applyStep("InsertionC");
			else
				delete alterObj.second;
			alterObj.second = 0;
			alterObj.first->set(0,0,0,0,0,0,0);
			return;
		}
		//Finished a move, resize, or copy operation. Check for consistency.
		graphNode* obj = alterObj.second;
		selectObj* sObj = alterObj.first;
		if (obj->isCut)
		{
			//Boundary checking
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			graphNode* p2 = (*displayItr)->getLowestSubtree(obj->x2,obj->y2);
			graphNode* p3 = (*displayItr)->getLowestSubtree(obj->x1,obj->y2);
			graphNode* p4 = (*displayItr)->getLowestSubtree(obj->x2,obj->y1);
			if (p1 != p2 || p1 != p3 || p1 != p4)
			{
				ttip.setText("Not contained entirely by one subtree.");
				ttip.setColor(1.0,0.0,0.0);
				if (sObj->type == 3) //Copy operation
					delete obj;
				else if (sObj->type == 2) //Resize operation
				{
					if (abs(sObj->x - sObj->iX1) < SELBOUND)
					{
						obj->x1 = sObj->iX1;
						if (abs(sObj->y - sObj->iY1) < SELBOUND)
							obj->y1 = sObj->iY1;
						else if (abs(sObj->y - sObj->iY2) < SELBOUND)
							obj->y2 = sObj->iY2;
					}
					else if (abs(sObj->x - sObj->iX2) < SELBOUND)
					{
						obj->x2 = sObj->iX2;
						if (abs(sObj->y - sObj->iY1) < SELBOUND)
							obj->y1 = sObj->iY1;
						else if (abs(sObj->y - sObj->iY2) < SELBOUND)
							obj->y2 = sObj->iY2;
					}
					else if (abs(sObj->y - sObj->iY1) < SELBOUND)
						obj->y1 = sObj->iY1;
					else if (abs(sObj->y - sObj->iY2) < SELBOUND)
						obj->y2 = sObj->iY2;
				}
				else if (sObj->subtree)
					obj->moveAll(sObj->iX1,sObj->iY1);
				else
					obj->move(sObj->iX1,sObj->iY1);
				sObj->set(0,0,0,0,0,0,0);
				alterObj.second = 0;
				return;
			}
			if (p1 == obj)
			{
				//Cut object think itself is its new parent.
				//Really it just has the same parent as before.
				p1 = obj->parent;
			}
			if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Cut or contained object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				if (sObj->type == 3)
					delete obj;
				else if (sObj->type == 2) //Resize operation
				{
					if (abs(sObj->x - sObj->iX1) < SELBOUND)
					{
						obj->x1 = sObj->iX1;
						if (abs(sObj->y - sObj->iY1) < SELBOUND)
							obj->y1 = sObj->iY1;
						else if (abs(sObj->y - sObj->iY2) < SELBOUND)
							obj->y2 = sObj->iY2;
					}
					else if (abs(sObj->x - sObj->iX2) < SELBOUND)
					{
						obj->x2 = sObj->iX2;
						if (abs(sObj->y - sObj->iY1) < SELBOUND)
							obj->y1 = sObj->iY1;
						else if (abs(sObj->y - sObj->iY2) < SELBOUND)
							obj->y2 = sObj->iY2;
					}
					else if (abs(sObj->y - sObj->iY1) < SELBOUND)
						obj->y1 = sObj->iY1;
					else if (abs(sObj->y - sObj->iY2) < SELBOUND)
						obj->y2 = sObj->iY2;
				}
				else if (sObj->subtree)
					obj->moveAll(sObj->iX1,sObj->iY1);
				else
					obj->move(sObj->iX1,sObj->iY1);
				sObj->set(0,0,0,0,0,0,0);
				alterObj.second = 0;
				return;
			}
			//Check to see if obj no longer contains some child.
			//We'll actually deal with this later (need to make
			//sure the operation is completely valid before
			//doing any alterations).
			bool loseChildren = false;
			std::list<graphNode*>::iterator cItr = obj->children.begin();
			while (cItr != obj->children.end())
			{
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 != pn2 || pn1 != pn3 || pn1 != pn4)
					{
						ttip.setText("A child would be partially contained by different subtrees.");
						ttip.setColor(1.0,0.0,0.0);
						if (sObj->type == 3)
							delete obj;
						else if (sObj->type == 2) //Resize operation
						{
							if (abs(sObj->x - sObj->iX1) < SELBOUND)
							{
								obj->x1 = sObj->iX1;
								if (abs(sObj->y - sObj->iY1) < SELBOUND)
									obj->y1 = sObj->iY1;
								else if (abs(sObj->y - sObj->iY2) < SELBOUND)
									obj->y2 = sObj->iY2;
							}
							else if (abs(sObj->x - sObj->iX2) < SELBOUND)
							{
								obj->x2 = sObj->iX2;
								if (abs(sObj->y - sObj->iY1) < SELBOUND)
									obj->y1 = sObj->iY1;
								else if (abs(sObj->y - sObj->iY2) < SELBOUND)
									obj->y2 = sObj->iY2;
							}
							else if (abs(sObj->y - sObj->iY1) < SELBOUND)
								obj->y1 = sObj->iY1;
							else if (abs(sObj->y - sObj->iY2) < SELBOUND)
								obj->y2 = sObj->iY2;
						}
						else if (sObj->subtree)
							obj->moveAll(sObj->iX1,sObj->iY1);
						else
							obj->move(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
					if (!pn1 && !premiseMode)
					{
						ttip.setText("All children must remain inside this cut.");
						ttip.setColor(1.0,0.0,0.0);
						obj->moveAll(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
					else if (!pn1)
					{
						loseChildren = true;
						cItr++;
						continue;
					}
				}
				else
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					if (!pn1 && !premiseMode)
					{
						ttip.setText("All children must remain inside this cut.");
						ttip.setColor(1.0,0.0,0.0);
						obj->moveAll(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
					else if (!pn1)
					{
						loseChildren = true;
						cItr++;
						continue;
					}
				}
				cItr++;
			}

			//Check to see if obj now has a new parent.
			//If so, change the parent (and parent's children).
			bool changeParent = false;
			std::string inf = "none";
			if (p1 != obj->parent)
			{
				if (!premiseMode)
				{
					//Can assume it's a move operation.
					//Copying in InfMode is handled above, and 
					//a resize can't cause this.
					if (verifyStep("Iteration"))
						inf = "Iteration";
					else if (verifyStep("InsertionC"))
						inf = "InsertionC";
					if (inf.compare("none") == 0)
					{
						obj->moveAll(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
				}
				else
					changeParent = true;
			}
			//Check to see if obj now contains new objects.
			//If so, and not in premise mode, fail.
			bool kidnap = false;
			cItr = p1->children.begin(); //Current parent is p1 whether it was before or not.
			while (cItr != p1->children.end())
			{
				if ((*cItr) == obj)
				{
					cItr++;
					continue;
				}
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 && pn2 && pn3 && pn4)
					{
						if (!premiseMode)
						{
							ttip.setText("Cut cannot steal existing children.");
							ttip.setColor(1.0,0.0,0.0);
							obj->moveAll(sObj->iX1,sObj->iY1);
							sObj->set(0,0,0,0,0,0,0);
							alterObj.second = 0;
							return;
						}
						else
							kidnap = true;
					}
					else if (pn1 || pn2 || pn3 || pn4)
					{
						ttip.setText("Cut would partially contain another cut.");
						ttip.setColor(1.0,0.0,0.0);
						if (sObj->type == 3)
							delete obj;
						else if (sObj->type == 2) //Resize operation
						{
							if (abs(sObj->x - sObj->iX1) < SELBOUND)
							{
								obj->x1 = sObj->iX1;
								if (abs(sObj->y - sObj->iY1) < SELBOUND)
									obj->y1 = sObj->iY1;
								else if (abs(sObj->y - sObj->iY2) < SELBOUND)
									obj->y2 = sObj->iY2;
							}
							else if (abs(sObj->x - sObj->iX2) < SELBOUND)
							{
								obj->x2 = sObj->iX2;
								if (abs(sObj->y - sObj->iY1) < SELBOUND)
									obj->y1 = sObj->iY1;
								else if (abs(sObj->y - sObj->iY2) < SELBOUND)
									obj->y2 = sObj->iY2;
							}
							else if (abs(sObj->y - sObj->iY1) < SELBOUND)
								obj->y1 = sObj->iY1;
							else if (abs(sObj->y - sObj->iY2) < SELBOUND)
								obj->y2 = sObj->iY2;
						}
						else if (sObj->subtree)
							obj->moveAll(sObj->iX1,sObj->iY1);
						else
							obj->move(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
				}
				else
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					if (pn1)
					{
						if (!premiseMode)
						{
							ttip.setText("Cut cannot steal existing children.");
							ttip.setColor(1.0,0.0,0.0);
							obj->moveAll(sObj->iX1,sObj->iY1);
							sObj->set(0,0,0,0,0,0,0);
							alterObj.second = 0;
							return;
						}
						else
							kidnap = true;
					}
				}
				cItr++;
			}

			//Now that the operation is known to be valid,
			//actually perform the operation.
			if (premiseMode)
			{
				if (loseChildren)
				{
					cItr = obj->children.begin();
					graphNode* dChild;
					while (cItr != obj->children.end())
					{
						bool pnb = obj->contains((*cItr)->x1,(*cItr)->y1);
						if (!pnb)
						{
							graphNode* pn1 = (*displayItr)->getLowestSubtree((*cItr)->x1,(*cItr)->y1);
							dChild = (*cItr);
							dChild->parent = pn1;
							pn1->addChild(dChild);
							cItr++;
							obj->children.remove(dChild);
						}
						else
							cItr++;
					}
				}
				if (kidnap)
					obj->stealChildren(p1); //We've already checked for the failure cases.
				//std::cout << changeParent << " " << sObj->type << "\n";
				if (changeParent)
				{
					obj->parent->children.remove(obj);
					if (kidnap)
						(*displayItr)->insert(p1,obj);
					else
						(*displayItr)->add(p1,obj);
				}
				else if (kidnap && (sObj->type == 1 || sObj->type == 2))
				{
					//Remove any children stolen by this object from former parent.
					cItr = obj->children.begin();
					while (cItr != obj->children.end())
					{
						obj->parent->children.remove((*cItr));
						cItr++;
					}
				}
				else if (sObj->type == 3) //Copied object only thinks it has the right parent
				{
					if (kidnap)
						(*displayItr)->insert(p1,obj);
					else
						(*displayItr)->add(p1,obj);
					obj->setColor(0.0,0.0,0.0);
				}
				sObj->set(0,0,0,0,0,0,0);
				alterObj.second = 0;
			}
			else
			{
				if (inf.compare("none") != 0)
				{
					//Adding these is valid. Check to see if the
					//old ones can be removed.
					if (obj->getDepth() % 2 == 0 || obj->duplicates(obj) != 0)
					{
						applyStep(inf);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
					else
					{
						//Can't be removed. Fail.
						obj->moveAll(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
				}
				//Else case was already handled.
			}
		}
		else
		{
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				if (sObj->type == 3)
					delete obj;
				else
					obj->moveAll(sObj->iX1,sObj->iY1);
				sObj->set(0,0,0,0,0,0,0);
				alterObj.second = 0;
				return;
			}
			if (p1 != obj->parent) //New parent
			{
				if (premiseMode)
				{
					if (sObj->type == 1)
						obj->parent->children.remove(obj);
					obj->parent = p1;
					p1->addChild(obj);
				}
				else
				{
					std::string inf = "none";
					if (verifyStep("Iteration"))
						inf = "Iteration";
					else if (verifyStep("Insertion"))
						inf = "Insertion";
					if (inf.compare("none") != 0)
					{
						//Adding these is valid. Check to see if the
						//old ones can be removed.
						if (obj->getDepth() % 2 == 0 || obj->duplicates(obj) != 0)
						{
							applyStep(inf);
							sObj->set(0,0,0,0,0,0,0);
							alterObj.second = 0;
							return;
						}
						else
						{
							//Can't be removed. Fail.
							ttip.setText("Unable to remove object from old location.");
							ttip.setColor(1.0,0.0,0.0);
							obj->moveAll(sObj->iX1,sObj->iY1);
							sObj->set(0,0,0,0,0,0,0);
							alterObj.second = 0;
							return;
						}
					}
					else
					{
						//Can't be moved there. Fail.
						obj->moveAll(sObj->iX1,sObj->iY1);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
						return;
					}
				}
			}
			else if (sObj->type == 3)
			{
				//Same parent in a premiseMode copy.
				//The object thinks it has the right parent, but
				//no one else knows about it.
				obj->parent->addChild(obj);
				obj->setColor(0.0,0.0,0.0);
			}
			//Does not have a new parent.
			//Moved or copied with no collisions
			//and no adjustments needed.
			//Just leave it there.
			sObj->set(0,0,0,0,0,0,0);
			alterObj.second = 0;
		}
		sObj->set(0,0,0,0,0,0,0);
		sObj->original = 0;
		alterObj.second = 0;
		if (premiseMode)
			ttip.setText("Premise Drawing Mode.");
		ttip.setColor(0.9,0.1,0.7);
		return;
	}

	if (!premiseMode && nNode != 0 && button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		//Button release for a doublecut add.
		if (verifyStep("DoubleCutAdd"))
			applyStep("DoubleCutAdd");
		else
			delete nNode;
		nNode = 0;
	}

	if (premiseMode == true)
	{
		if (state == GLUT_UP) //Button release
		{
			if (button == GLUT_RIGHT_BUTTON && nNode != 0)
			{
				if (abs(nNode->x1 - nNode->x2) < 0.2 || abs(nNode->y1 - nNode->y2) < 0.2)
				{
					ttip.setText("Cut is too small.");
					glutSetWindow(tooltipWindow);
					glutPostRedisplay();
					delete nNode;	//Too small of a cut. Abort.
				}

				//Boundary checking
				graphNode* p1 = (*displayItr)->getLowestSubtree(nNode->x1,nNode->y1);
				graphNode* p2 = (*displayItr)->getLowestSubtree(nNode->x2,nNode->y2);
				graphNode* p3 = (*displayItr)->getLowestSubtree(nNode->x1,nNode->y2);
				graphNode* p4 = (*displayItr)->getLowestSubtree(nNode->x2,nNode->y1);

				if (p1 != p2 || p1 != p3 || p1 != p4)
				{
					ttip.setText("Cut is not contained entirely by one subtree.");
					glutSetWindow(tooltipWindow);
					glutPostRedisplay();
					delete nNode;	//Abort
				}
				else
				{
					if (nNode->stealChildren(p1))
					{
						nNode->setColor(0.0,0.0,0.0);
						(*displayItr)->insert(p1,nNode);
					}
					else
					{
						ttip.setText("Cut partially contains another subtree.");
						glutSetWindow(tooltipWindow);
						glutPostRedisplay();
						delete nNode;
					}
				}
				nNode = 0;
			}
			else if (button == GLUT_LEFT_BUTTON && nNode != 0)
			{
				std::pair<float,float> mLoc = getLoc(x,y);
				if (abs(mLoc.first - nNode->x1) > 0.15 || abs(mLoc.second - nNode->y1) > 0.15)
				{
					delete nNode;
					nNode = 0;
					return;
				}
				takeInTxt = true;
				ttip.setText("Enter object label.");
				glutSetWindow(tooltipWindow);
				glutPostRedisplay();
			}
		}
		else
		{
			if (button == GLUT_RIGHT_BUTTON) //Right-click
			{
				std::pair<float,float> mLoc = getLoc(x,y);
				graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);				
				if (sPick == (*displayItr)->root)
				{
					nNode = new graphNode();
					nNode->isCut = true;
					nNode->setColor(0.5,0.5,0.5);
					nNode->x1 = mLoc.first;
					nNode->y1 = mLoc.second;
					nNode->x2 = mLoc.first;
					nNode->y2 = mLoc.second;
				}
				else //Deselect object
				{
					int i = (*displayItr)->isSelected(sPick);
					if (i == 1)
						(*displayItr)->deselect(sPick,0.0,0.0,0.0);
					else if (i == 2)
						(*displayItr)->deselectSubtree(sPick,0.0,0.0,0.0);
				}
			}
			else if (button == GLUT_LEFT_BUTTON) //Left-click
			{
				std::pair<float,float> mLoc = getLoc(x,y);
				graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
				if (sPick == (*displayItr)->root)
				{
					nNode = new graphNode();
					nNode->x1 = mLoc.first;
					nNode->y1 = mLoc.second;
				}
				else //Already something there, select it
				{
					int i = (*displayItr)->isSelected(sPick);
					if ((mod&GLUT_ACTIVE_CTRL) == GLUT_ACTIVE_CTRL)
					{
						if (i == 1)
						{
							(*displayItr)->deselect(sPick,0.0,0.0,0.0);
							(*displayItr)->selectSubtree(sPick,1.0,0.0,0.0,0.0,0.0,1.0);
						}
						else if (i == 0)
							(*displayItr)->selectSubtree(sPick,1.0,0.0,0.0,0.0,0.0,1.0);
						alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 3);
						alterObj.first->subtree = true;
					}
					else if ((mod&GLUT_ACTIVE_SHIFT) == GLUT_ACTIVE_SHIFT && sPick->isCut)
					{
						alterObj.first->subtree = false;
						alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 2);
					}
					else
					{
						if (i == 0)
							(*displayItr)->select(sPick,1.0,0.0,0.0);
						alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 1);
						alterObj.first->subtree = false;
					}
					alterObj.first->original = sPick;
				}
			}
		}
		return;
	}
	if (state == GLUT_UP) return; //Button release, no selection changes

	if ((mod&GLUT_ACTIVE_CTRL) == GLUT_ACTIVE_CTRL) //Ctrl key is pressed
	{
		if (button == GLUT_LEFT_BUTTON) //Left-click
		{
			//Select subtree
			std::pair<float,float> mLoc = getLoc(x,y);
			graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
			if (sPick != (*displayItr)->root)
			{
				int i = (*displayItr)->isSelected(sPick);
				if (i == 1)
				{
					(*displayItr)->deselect(sPick,0.0,0.0,0.0);
					(*displayItr)->selectSubtree(sPick,1.0,0.0,0.0,0.0,0.0,1.0);
				}
				else if (i == 0)
					(*displayItr)->selectSubtree(sPick,1.0,0.0,0.0,0.0,0.0,1.0);
				alterObj.first->original = sPick;
				alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 3);
				alterObj.first->subtree = true;
			}
		}
	}
	else
	{
		if (button == GLUT_LEFT_BUTTON) //Left-click
		{
			//Select object
			std::pair<float,float> mLoc = getLoc(x,y);
			graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
			if (sPick != (*displayItr)->root)
			{
				int i = (*displayItr)->isSelected(sPick);
				if (i == 0)
					(*displayItr)->select(sPick,1.0,0.0,0.0);
				alterObj.first->original = sPick;
				if ((mod&GLUT_ACTIVE_SHIFT) == GLUT_ACTIVE_SHIFT && sPick->isCut)
					alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 2);
				else
					alterObj.first->set(mLoc.first, mLoc.second, sPick->x1, sPick->y1, sPick->x2, sPick->y2, 1);
				alterObj.first->subtree = false;
				if (i == 2)
					alterObj.first->subtree = true;
			}
		}
		else if (button == GLUT_RIGHT_BUTTON) //Right-click
		{
			//Deselect object
			std::pair<float,float> mLoc = getLoc(x,y);
			graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
			if (sPick != (*displayItr)->root)
			{
				int i = (*displayItr)->isSelected(sPick);
				if (i == 1)
					(*displayItr)->deselect(sPick,0.0,0.0,0.0);
				else if (i == 2)
					(*displayItr)->deselectSubtree(sPick,0.0,0.0,0.0);
			}
			else //Make a new doublecut
			{
				nNode = new graphNode();
				nNode->isCut = true;
				nNode->setColor(0.5,0.5,0.5);
				nNode->x1 = mLoc.first;
				nNode->y1 = mLoc.second;
				nNode->x2 = mLoc.first;
				nNode->y2 = mLoc.second;

				graphNode* nn2 = new graphNode();
				nn2->isCut = true;
				nn2->setColor(0.5,0.5,0.5);
				nn2->x1 = mLoc.first;
				nn2->y1 = mLoc.second;
				nn2->x2 = mLoc.first;
				nn2->y2 = mLoc.second;
				nn2->parent = nNode;
				nNode->addChild(nn2);
			}
		}
	}
}

//Mouse is moving while a button is pressed
//X and Y are pixel values on the window pane
//(0,0 is top left)
void MouseActMove(int x, int y)
{
	if (x < 0 || y < 0 || x > screenWidth || y > screenHeight)
		return; //Outside the window

	std::pair<float,float> mLoc = getLoc(x,y);
	if (premiseMode == true && nNode != 0)
	{
		nNode->x2 = mLoc.first;
		nNode->y2 = mLoc.second;
	}
	else if (!premiseMode && nNode != 0)
	{
		float bound = SELBOUND*4;
		graphNode* nn2 = nNode->children.front();
		nNode->x2 = mLoc.first;
		nNode->y2 = mLoc.second;
		if (mLoc.first < nNode->x1)
		{
			nn2->x1 = nNode->x1 - bound;
			nn2->x2 = mLoc.first + bound;
		}
		else
		{
			nn2->x1 = nNode->x1 + bound;
			nn2->x2 = mLoc.first - bound;
		}
		if (mLoc.second < nNode->y1)
		{
			nn2->y1 = nNode->y1 - bound;
			nn2->y2 = mLoc.second + bound;
		}
		else
		{
			nn2->y1 = nNode->y1 + bound;
			nn2->y2 = mLoc.second - bound;
		}
	}
	else if (alterObj.second == 0 && alterObj.first->type != 0) //Determine the type of object altering
	{
		if (alterObj.first->type == 3) //Copy
		{
			std::cout << "Copy\n";
			alterObj.second = new graphNode(alterObj.first->original);
		}
		else if (alterObj.first->type == 2) //Resize
		{
			std::cout << "Resize\n";
			alterObj.second = alterObj.first->original;
		}
		else //Move
		{
			std::cout << "Move\n";
			alterObj.second = alterObj.first->original;
		}
	}
	else if (alterObj.second != 0) //Moving with a selection not a new object
	{
		selectObj* obj = alterObj.first;
		if (obj->type == 1 || obj->type == 3) //Move or Copy object
		{
			//int i = (*displayItr)->isSelected(obj->original);
			//if (i == 2)
			if (alterObj.first->subtree)
				alterObj.second->moveAll(mLoc.first + (obj->iX1 - obj->x), mLoc.second + (obj->iY1 - obj->y));
			else
				alterObj.second->move(mLoc.first + (obj->iX1 - obj->x), mLoc.second + (obj->iY1 - obj->y));
		}
		else if (obj->type == 2) //Resize object
		{
			if (abs(obj->x - obj->iX1) < SELBOUND)
			{
				alterObj.second->x1 = mLoc.first;
				if (abs(obj->y - obj->iY1) < SELBOUND)
					alterObj.second->y1 = mLoc.second;
				else if (abs(obj->y - obj->iY2) < SELBOUND)
					alterObj.second->y2 = mLoc.second;
			}
			else if (abs(obj->x - obj->iX2) < SELBOUND)
			{
				alterObj.second->x2 = mLoc.first;
				if (abs(obj->y - obj->iY1) < SELBOUND)
					alterObj.second->y1 = mLoc.second;
				else if (abs(obj->y - obj->iY2) < SELBOUND)
					alterObj.second->y2 = mLoc.second;
			}
			else if (abs(obj->y - obj->iY1) < SELBOUND)
				alterObj.second->y1 = mLoc.second;
			else if (abs(obj->y - obj->iY2) < SELBOUND)
				alterObj.second->y2 = mLoc.second;
		}
	}
}

//Mouse is moving while no button is pressed
void MousePassMove(int x, int y)
{
}

//Get the location of the mouse in the drawing plane
std::pair<float,float> getLoc(int mx, int my)
{
	float vScale = 0.75*tan((float)45/2)*cameraZ;
	float hScale = ((float)screenWidth/(float)screenHeight)*vScale;
	float x, y;
	x = cameraX + hScale*(mx-screenWidth/2)/(screenWidth/2);
	y = cameraY - vScale*(my-screenHeight/2)/(screenHeight/2);

	std::pair<float,float> loc;
	loc.first = x;
	loc.second = y;
	return loc;
}

void animate(int value)
{
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
	glutSetWindow(mainWindow);
	glutPostRedisplay();
	glutTimerFunc(10, animate, 0);
}

void undo()
{
	if (displayItr != undoBuffer.begin())
		displayItr--;
	glutSetWindow(mainWindow);
	glutPostRedisplay();
}

void redo()
{
	std::list<graphTree*>::iterator itr = displayItr;
	itr++;
	if (itr != undoBuffer.end())
		displayItr++;
	glutSetWindow(mainWindow);
	glutPostRedisplay();
}

void reset()
{
	undoBuffer.reinit(5);
	displayItr = undoBuffer.begin();

	premiseMode = true;
	cameraX = 0;
	cameraY = 0;
	cameraZ = 15;

	ttip.setText("Premise Drawing Mode.");
	ttip.setColor(0.9,0.1,0.7);
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
}


//Inference Rule Functions

bool verifyStep(std::string rule)
{
	bool verify = true;
	std::list<std::pair<graphNode*,bool> >::iterator itr = (*displayItr)->selection.begin();
	if (rule.compare("DoubleCutRemove") == 0)
	{
		std::list<graphNode*> used;
		while (itr != (*displayItr)->selection.end())
		{
			if ((*itr).first->isCut == false || (*itr).second == true)
			{
				ttip.setText("At least one selected object is an atom or subtree.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			if (!contains(used,(*itr).first))
			{
				used.push_back((*itr).first);
				//Parent is an unused selected cut
				if ((*displayItr)->isSelected((*itr).first->parent) == 1 && 
					(*itr).first->parent->isCut == true && 
					!contains(used,(*itr).first->parent))
				{
					if ((*itr).first->children.size() == 1 && 
						(*displayItr)->isSelected((*itr).first->children.front()) == 1 &&
						(*itr).first->children.front()->isCut == true &&
						!contains(used,(*itr).first->children.front()))
					{
						//There's a choice, so make a recursive call to enable backtracking.
						used.push_back((*itr).first->parent);
						if (!verifyRec(rule, used, itr))
						{
							used.pop_back();	//Don't select parent
							used.push_back((*itr).first->children.front());
							if (!verifyRec(rule, used, itr))
							{
								//Neither works. Failure.
								ttip.setText("Not all selections form double cuts.");
								ttip.setColor(1.0,0.0,0.0);
								verify = false;
								break;
							}
							else //Verifies, don't waste time re-verifying
								return true;
						}
						else //Verifies, don't waste time re-verifying
							return true;
					}
					else //Only the parent works
						used.push_back((*itr).first->parent);
				}
				//Only the child is an unused selected cut
				else if ((*itr).first->children.size() == 1 && 
					(*displayItr)->isSelected((*itr).first->children.front()) == 1 &&
					(*itr).first->children.front()->isCut == true &&
					!contains(used,(*itr).first->children.front()))
				{
					used.push_back((*itr).first->children.front());
				}
				else
				{
					//Neither works. Failure.
					ttip.setText("Not all selections form double cuts.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
			}
			itr++;
		}
	}
	else if (rule.compare("Deiteration") == 0)
	{
		if ((*displayItr)->selection.size() < 2)
		{
			ttip.setText("There must be at least two selections for Deiteration.");
			ttip.setColor(1.0,0.0,0.0);
			verify = false;
		}
		else
		{
			std::list<std::pair<graphNode*,bool> >::iterator bItr;
			graphNode* base = 0;
			int bDepth = -1;
			while (itr != (*displayItr)->selection.end())	//Find lowest depth selection
			{
				if (base == 0 || (*itr).first->getDepth() < bDepth)
				{
					base = (*itr).first;
					bItr = itr;
					bDepth = (*itr).first->getDepth();
				}
				itr++;
			}

			itr = (*displayItr)->selection.begin();
			while (itr != (*displayItr)->selection.end())
			{
				if ((*bItr).second != (*itr).second)
				{
					ttip.setText("All selections must be of the same type (subtree or object).");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
				if (base->eqo((*itr).first) == false)	//Check equivalence
				{
					ttip.setText("All selections must have the same label or be empty cuts.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
				if ((*itr).first->inSubtreeOf(base) == false)	//Check same subtree
				{
					ttip.setText("All selections must be in the same subtree as lowest depth selection.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
				if ((*itr).first->isCut == true && (*itr).second == false && (*itr).first->children.size() > 0)
				{
					ttip.setText("Cuts must either be empty or selected as subtrees.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
				itr++;
			}
		}
	}
	else if (rule.compare("Erasure") == 0)
	{
		while (itr != (*displayItr)->selection.end())
		{
			if ((*itr).first->getDepth() % 2 == 1) //Depth is odd
			{
				ttip.setText("At least one selected object is at an odd level.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			if ((*itr).first->isCut == true && (*itr).second == false && (*itr).first->children.size() > 0)
			{	//Selection is a non-empty cut selected as an object
				ttip.setText("Cuts must either be empty or selected as subtrees.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			itr++;
		}
	}
	else if (rule.compare("Iteration") == 0)
	{
		graphNode* obj = alterObj.second;
		selectObj* sObj = alterObj.first;
		if (obj->isCut)
		{
			//Boundary checking
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			graphNode* p2 = (*displayItr)->getLowestSubtree(obj->x2,obj->y2);
			graphNode* p3 = (*displayItr)->getLowestSubtree(obj->x1,obj->y2);
			graphNode* p4 = (*displayItr)->getLowestSubtree(obj->x2,obj->y1);
			if (p1 != p2 || p1 != p3 || p1 != p4)
			{
				ttip.setText("Not contained entirely by one subtree.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			else if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Cut or contained object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			std::list<graphNode*>::iterator cItr = obj->children.begin();
			while (verify && cItr != obj->children.end())
			{
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 != pn2 || pn1 != pn3 || pn1 != pn4)
					{
						ttip.setText("A child would be partially contained by different subtrees.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
					else if (!pn1)
					{
						ttip.setText("All children must remain inside this cut.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				else
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					if (!pn1)
					{
						ttip.setText("All children must remain inside this cut.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				cItr++;
			}
			if (verify && p1 != obj->parent)
			{
				//Object has a new parent. Check for validity.
				if (!(p1->inSubtree(obj->parent)))
				{
					ttip.setText("Object can only be iterated to a lower level in the same subtree.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
				}
			}
			//Check if it now contains new objects.
			cItr = p1->children.begin(); //Current parent is p1 whether it was before or not.
			while (verify && cItr != p1->children.end())
			{
				if ((*cItr) == obj)
				{
					cItr++;
					continue;
				}
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 && pn2 && pn3 && pn4)
					{
						ttip.setText("Cut cannot steal existing children.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
					else if (pn1 || pn2 || pn3 || pn4)
					{
						ttip.setText("Cut would partially contain another cut.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				else
				{
					if(obj->contains((*cItr)->x1,(*cItr)->y1))
					{
						ttip.setText("Cut cannot steal existing children.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				cItr++;
			}
		}
		else
		{
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			else if (p1 != obj->parent) //New parent
			{
				if (!(p1->inSubtree(obj->parent)))
				{
					ttip.setText("Object can only be iterated to a lower level in the same subtree.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
				}
			}
		}
	}
	else if (rule.compare("InsertionC") == 0)
	{
		//Special case of Insertion where the object to be
		//added already exists and is being moved or copied
		//to the location rather than drawn.
		graphNode* obj = alterObj.second;
		selectObj* sObj = alterObj.first;
		if (obj->isCut)
		{
			//Boundary checking
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			graphNode* p2 = (*displayItr)->getLowestSubtree(obj->x2,obj->y2);
			graphNode* p3 = (*displayItr)->getLowestSubtree(obj->x1,obj->y2);
			graphNode* p4 = (*displayItr)->getLowestSubtree(obj->x2,obj->y1);
			if (p1 != p2 || p1 != p3 || p1 != p4)
			{
				ttip.setText("Not contained entirely by one subtree.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			else if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Cut or contained object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			std::list<graphNode*>::iterator cItr = obj->children.begin();
			bool loseChildren = false;
			while (verify && cItr != obj->children.end())
			{
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 != pn2 || pn1 != pn3 || pn1 != pn4)
					{
						ttip.setText("A child would be partially contained by different subtrees.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
					else if (!pn1)
						loseChildren = true;
				}
				else
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					if (!pn1)
						loseChildren = true;
				}
				cItr++;
			}
			if (verify)
			{
				//Check for validity.
				if (p1->getDepth() % 2 != 0)
				{
					ttip.setText("Object can only be Inserted at an odd level.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
				}
			}
			//Check if it now contains new objects.
			cItr = p1->children.begin(); //Current parent is p1 whether it was before or not.
			while (verify && cItr != p1->children.end())
			{
				if ((*cItr) == obj)
				{
					cItr++;
					continue;
				}
				if ((*cItr)->isCut)
				{
					bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
					bool pn2 = obj->contains((*cItr)->x1,(*cItr)->y2);
					bool pn3 = obj->contains((*cItr)->x2,(*cItr)->y2);
					bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
					if (pn1 && pn2 && pn3 && pn4)
					{
						ttip.setText("Cut cannot steal existing children.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
					else if (pn1 || pn2 || pn3 || pn4)
					{
						ttip.setText("Cut would partially contain another cut.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				else
				{
					if(obj->contains((*cItr)->x1,(*cItr)->y1))
					{
						ttip.setText("Cut cannot steal existing children.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				cItr++;
			}
		}
		else
		{
			graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
			if (obj->proxCol(p1,SELBOUND))
			{
				//Too close to something else. Fail.
				ttip.setText("Object is too close to another object.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
			else if (p1->getDepth() % 2 != 0)
			{
				ttip.setText("Object can only be Inserted at an odd level.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
			}
		}
	}
	else if (rule.compare("DoubleCutAdd") == 0)
	{
		graphNode* nn2 = nNode->children.front();
		if (abs(nn2->x1 - nn2->x2) < SELBOUND*4 || abs(nn2->y1 - nn2->y2) < SELBOUND*4)
		{
			ttip.setText("Cut is too small.");
			ttip.setColor(1.0,0.0,0.0);
			verify = false;
		}
		//Boundary checking
		graphNode* p1 = (*displayItr)->getLowestSubtree(nNode->x1,nNode->y1);
		graphNode* p2 = (*displayItr)->getLowestSubtree(nNode->x2,nNode->y2);
		graphNode* p3 = (*displayItr)->getLowestSubtree(nNode->x1,nNode->y2);
		graphNode* p4 = (*displayItr)->getLowestSubtree(nNode->x2,nNode->y1);

		if (p1 != p2 || p1 != p3 || p1 != p4)
		{
			ttip.setText("Cut is not contained entirely by one subtree.");
			ttip.setColor(1.0,0.0,0.0);
			verify = false;
		}
		else
		{
			//Check for children that would need to be stolen.
			std::list<graphNode*>::iterator cItr = p1->children.begin();
			while (cItr != p1->children.end())
			{
				if ((*cItr)->isCut)
				{
					int corners = 0;
					if (nn2->contains((*cItr)->x1,(*cItr)->y1))
						corners++;
					if (nn2->contains((*cItr)->x2,(*cItr)->y2))
						corners++;
					if (nn2->contains((*cItr)->x1,(*cItr)->y2))
						corners++;
					if (nn2->contains((*cItr)->x2,(*cItr)->y1))
						corners++;
					if (corners != 4 && corners > 0)
					{
						ttip.setText("Cut partially contains another object.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
					else if (corners != 4)
					{
						//Make sure that the outer cut doesn't contain it.
						if (nNode->contains((*cItr)->x1,(*cItr)->y1))
							corners++;
						if (nNode->contains((*cItr)->x2,(*cItr)->y2))
							corners++;
						if (nNode->contains((*cItr)->x1,(*cItr)->y2))
							corners++;
						if (nNode->contains((*cItr)->x2,(*cItr)->y1))
							corners++;
						if (corners > 0)
						{
							ttip.setText("Object is between the cuts.");
							ttip.setColor(1.0,0.0,0.0);
							verify = false;
							break;
						}
					}
				}
				else
				{
					if (!(nn2->contains((*cItr)->x1,(*cItr)->y1)) && 
						nNode->contains((*cItr)->x1,(*cItr)->y1))
					{
						ttip.setText("Object is between the cuts.");
						ttip.setColor(1.0,0.0,0.0);
						verify = false;
						break;
					}
				}
				cItr++;
			}
		}
	}
	else
	{
		ttip.setText("Unknown inference rule");
		ttip.setColor(1.0,0.0,0.0);
	}
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
	return verify;
}

bool verifyRec(std::string rule, std::list<graphNode*> used, std::list<std::pair<graphNode*,bool> >::iterator itr)
{
	bool verify = true;
	itr++;
	if (rule.compare("DoubleCutRemove") == 0)
	{
		while (itr != (*displayItr)->selection.end())
		{
			if ((*itr).first->isCut == false || (*itr).second == true)
			{
				ttip.setText("At least one selected object is an object or subtree.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			if (!contains(used,(*itr).first))
			{
				used.push_back((*itr).first);
				//Parent is an unused selected cut
				if ((*displayItr)->isSelected((*itr).first->parent) == 1 && 
					(*itr).first->parent->isCut == true && 
					!contains(used,(*itr).first->parent))
				{
					if ((*itr).first->children.size() == 1 && 
						(*displayItr)->isSelected((*itr).first->children.front()) == 1 &&
						(*itr).first->children.front()->isCut == true &&
						!contains(used,(*itr).first->children.front()))
					{
						//There's a choice, so make a recursive call to enable backtracking.
						used.push_back((*itr).first->parent);
						if (!verifyRec(rule, used, itr))
						{
							used.pop_back();	//Don't select parent
							used.push_back((*itr).first->children.front());
							if (!verifyRec(rule, used, itr))
							{
								//Neither works. Failure.
								ttip.setText("Not all selections form double cuts.");
								ttip.setColor(1.0,0.0,0.0);
								verify = false;
								break;
							}
							else //Verifies, don't waste time re-verifying
								return true;
						}
						else //Verifies, don't waste time re-verifying
							return true;
					}
					else //Only the parent works
						used.push_back((*itr).first->parent);
				}
				//Only the child is an unused selected cut
				else if ((*itr).first->children.size() == 1 && 
					(*displayItr)->isSelected((*itr).first->children.front()) == 1 &&
					(*itr).first->children.front()->isCut == true &&
					!contains(used,(*itr).first->children.front()))
				{
					used.push_back((*itr).first->children.front());
				}
				else
				{
					//Neither works. Failure.
					ttip.setText("Not all selections form double cuts.");
					ttip.setColor(1.0,0.0,0.0);
					verify = false;
					break;
				}
			}
			itr++;
		}
	}
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
	return verify;
}

//Apply an inference rule.
//Assumes the rule is already verified as being correctly
//applicable in this situation.
void applyStep(std::string rule)
{
	//Note, displayItr will be pointing at the NEW tree after these steps:
	graphTree* nTree = new graphTree((*displayItr));
	graphTree* bump = undoBuffer.write(displayItr,nTree);
	if (bump != 0)
	{
		//Save old tree
	}

	if (rule.compare("DoubleCutRemove") == 0)
	{
		(*displayItr)->deleteSelections();
		ttip.setText("Double Cut Deletion successful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else if (rule.compare("Deiteration") == 0)
	{
		std::list<std::pair<graphNode*,bool> >::iterator itr = (*displayItr)->selection.begin();
		std::list<std::pair<graphNode*,bool> >::iterator bItr;
		graphNode* base = 0;
		int bDepth = -1;
		while (itr != (*displayItr)->selection.end())	//Find lowest depth selection
		{
			if (base == 0 || (*itr).first->getDepth() < bDepth)
			{
				base = (*itr).first;
				bItr = itr;
				bDepth = (*itr).first->getDepth();
			}
			itr++;
		}
		//Deselect that base so it doesn't get deleted
		if ((*bItr).second == false)
			(*displayItr)->deselect(base,0.0,0.0,0.0);
		else
			(*displayItr)->deselectSubtree(base,0.0,0.0,0.0);
		(*displayItr)->deleteSelections();
		ttip.setText("Deiteration successful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else if (rule.compare("Erasure") == 0)
	{
		(*displayItr)->deleteSelections();
		ttip.setText("Erasure successful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else if (rule.compare("Iteration") == 0)
	{
		if (alterObj.first->type == 3) //Copy operation
		{
			//New object isn't even in the old tree.
			//Just add it to the new one.
			graphNode* p1 = (*displayItr)->getLowestSubtree(alterObj.second->x1,alterObj.second->y1);
			alterObj.second->parent = p1;
			p1->addChild(alterObj.second);
			alterObj.second->setColor(0.0,0.0,0.0);
			if (alterObj.first->subtree)
				(*displayItr)->deselectSubtree((*displayItr)->findEqNode(alterObj.first->original),0.0,0.0,0.0);
			else
				(*displayItr)->deselect((*displayItr)->findEqNode(alterObj.first->original),0.0,0.0,0.0);
		}
		else //Move operation
		{
			graphNode* node = (*displayItr)->findEqNode(alterObj.second);
			graphNode* p1 = (*displayItr)->getLowestSubtree(node->x1,node->y1);
			if (p1 != node->parent)
			{
				node->parent->children.remove(node);
				node->parent = p1;
				p1->addChild(node);
			}
			if (alterObj.first->subtree)
				(*displayItr)->deselectSubtree(node,0.0,0.0,0.0);
			else
				(*displayItr)->deselect(node,0.0,0.0,0.0);
			alterObj.second->moveAll(alterObj.first->iX1,alterObj.first->iY1);
		}
		alterObj.first->set(0,0,0,0,0,0,0);
		alterObj.second = 0;
		ttip.setText("Iteration successful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else if (rule.compare("InsertionC") == 0)
	{
		if (alterObj.first->type == 3) //Copy operation
		{
			//New object isn't even in the old tree.
			//Just add it to the new one.
			graphNode* p1 = (*displayItr)->getLowestSubtree(alterObj.second->x1,alterObj.second->y1);
			alterObj.second->parent = p1;
			p1->addChild(alterObj.second);
			alterObj.second->setColor(0.0,0.0,0.0);
			if (alterObj.first->subtree)
				(*displayItr)->deselectSubtree((*displayItr)->findEqNode(alterObj.first->original),0.0,0.0,0.0);
			else
				(*displayItr)->deselect((*displayItr)->findEqNode(alterObj.first->original),0.0,0.0,0.0);
		}
		else //Move operation
		{
			graphNode* node = (*displayItr)->findEqNode(alterObj.second);
			graphNode* p1 = (*displayItr)->getLowestSubtree(node->x1,node->y1);
			if (p1 != node->parent)
			{
				node->parent->children.remove(node);
				node->parent = p1;
				p1->addChild(node);
			}
			if (alterObj.first->subtree)
				(*displayItr)->deselectSubtree(node,0.0,0.0,0.0);
			else
				(*displayItr)->deselect(node,0.0,0.0,0.0);
			alterObj.second->moveAll(alterObj.first->iX1,alterObj.first->iY1);
		}
		alterObj.first->set(0,0,0,0,0,0,0);
		alterObj.second = 0;
		ttip.setText("Insertion successful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else if (rule.compare("DoubleCutAdd") == 0)
	{
		graphNode* p1 = (*displayItr)->getLowestSubtree(nNode->x1,nNode->y1);
		graphNode* nn2 = nNode->children.front();
		nn2->stealChildren(p1);
		(*displayItr)->insert(p1,nn2);
		nn2->parent = nNode;
		nn2->setColor(0.0,0.0,0.0);
		(*displayItr)->insert(p1,nNode);
		nNode->setColor(0.0,0.0,0.0);
		ttip.setText("Double Cut Addtion sucessful.");
		ttip.setColor(0.0,1.0,0.0);
	}
	else
	{
		ttip.setText("Unknown inference.");
		ttip.setColor(1.0,0.0,0.0);
	}
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
}

bool contains(std::list<graphNode*> lst, graphNode* obj)
{
	std::list<graphNode*>::iterator itr = lst.begin();
	while (itr != lst.end())
	{
		if ((*itr) == obj)
			return true;
		itr++;
	}
	return false;
}