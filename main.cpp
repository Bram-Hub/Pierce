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
float cameraX, cameraY, cameraZ, cameraVX, cameraVY;
int screenWidth = 1024, screenHeight = 768;

displaybuffer undoBuffer;
std::list<graphTree*>::iterator displayItr;
toolTip ttip;
bool premiseMode = true, takeInTxt = false, insertionMode = false;
graphNode* nNode = 0;
int mainWindow, tooltipWindow;
char inputText[128];
unsigned int inTxtPos;

std::pair<selectObj*,graphNode*> alterObj;
std::list<graphNode*> insertionCuts;
std::list<graphNode*> insertionChildren;

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
graphNode* colParent(graphNode* obj);
int colChildren(graphNode* obj);
int colSteal(graphNode* obj, graphNode* p1);
void fail(int code);
void abort(int code);

void animate(int value);

void undo();
void redo();
void reset();

//Inference functions
bool verifyStep(std::string rule);
bool verifyRec(std::string rule, std::list<graphNode*> used, std::list<std::pair<graphNode*,bool> >::iterator itr);
bool verifyIteration();
bool verifyInsertionC();
bool verifyDCutAdd();
void applyStep(std::string rule);

bool insertInArea(graphNode* obj);
bool insertIsArea(graphNode* obj);
bool insertIsOld(graphNode* obj);

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
	cameraVX = 0;
	cameraVY = 0;
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

	if (key == 'x' && mod == GLUT_ACTIVE_ALT) exit(0);
	if (key == 'n' && mod == GLUT_ACTIVE_ALT) reset();
	if (key == 'p' && mod == GLUT_ACTIVE_ALT) (*displayItr)->debugPrint();

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
			if (insertionMode)
				ttip.setText("Insertion Mode.");
			else
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
			if (insertionMode)
				ttip.setText("Insertion Mode.");
			else
				ttip.setText("Premise Drawing Mode.");
			glutSetWindow(tooltipWindow);
			glutPostRedisplay();
		}
		return;
	}

	if (premiseMode)
	{
		if (key == 'r' && mod == GLUT_ACTIVE_ALT)
		{
			premiseMode = false;
			if (insertionMode)
			{
				insertionMode = false;
				insertionCuts.clear();
				insertionChildren.clear();
			}
			ttip.setText("Inference Mode.");
			ttip.setColor(0.3,0.7,0.9);
			glutSetWindow(tooltipWindow);
			glutPostRedisplay();
		}
		if (key == 127) //Delete
		{
			if (insertionMode)
			{
				std::list<std::pair<graphNode*,bool> >::iterator itr = (*displayItr)->selection.begin();
				while (itr != (*displayItr)->selection.end())
				{
					if (insertInArea((*itr).first) == false || insertIsOld((*itr).first))
					{
						ttip.setText("Cannot delete existing objects when Inserting.");
						ttip.setColor(1.0,0.0,0.0);
						glutSetWindow(tooltipWindow);
						glutPostRedisplay();
						return;
					}
					itr++;
				}
			}
			(*displayItr)->deleteSelections();
			(*displayItr)->debugPrint();
		}
		return;
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
		if (key == 13) //Enter
		{
			if (verifyStep("Insertion"))
			{
				applyStep("Insertion");
			}
		}
		return;
	}
}

void KeyboardSpec(int key, int x, int y)
{
}

//Return the parent the object obj should have if the graph
//is structured properly. NULL means that it would have
//multiple parents, which means the location is invalid.
graphNode* colParent(graphNode* obj)
{
	graphNode* p1 = (*displayItr)->getLowestSubtree(obj->x1,obj->y1);
	if (obj->isCut)
	{
		graphNode* p2 = (*displayItr)->getLowestSubtree(obj->x2,obj->y2);
		graphNode* p3 = (*displayItr)->getLowestSubtree(obj->x1,obj->y2);
		graphNode* p4 = (*displayItr)->getLowestSubtree(obj->x2,obj->y1);
		if (p1 != p2 || p1 != p3 || p1 != p4)
			return (graphNode*)0;
		//Cut object may think it is the new parent,
		//but that just means it's still the old one.
		else if (p1 == obj)
			p1 = obj->parent;
	}
	return p1;
}

//Determine if there are children the object the object should no
//longer have. Returns 0 if it isn't a cut or loses no children.
//Returns 1 if it does, and 2 if one is partially contained.
int colChildren(graphNode* obj)
{
	if (obj->isCut == false)
		return 0;
	std::list<graphNode*>::iterator cItr = obj->children.begin();
	int ret = 0;
	while (cItr != obj->children.end())
	{
		bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
		if (!pn1)
			ret = 1;
		if ((*cItr)->isCut)
		{
			bool pn2 = obj->contains((*cItr)->x2,(*cItr)->y2);
			bool pn3 = obj->contains((*cItr)->x1,(*cItr)->y2);
			bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
			if (pn1 != pn2 || pn1 != pn3 || pn1 != pn4)
				return 2; //Object partially contains a child
			if (obj->proxCol2((*cItr),SELBOUND))
				return 2;
		}
		cItr++;
	}
	return ret;
}

//Determine if the object now contains new children that
//formerly belonged to p1. Returns 0 if it isn't a cut 
//or contains no new children. Returns 1 if it does, and
//2 if something is partially contained.
int colSteal(graphNode* obj, graphNode* p1)
{
	if (obj->isCut == false)
		return 0;
	std::list<graphNode*>::iterator cItr = p1->children.begin();
	int ret = 0;
	while (cItr != p1->children.end())
	{
		if (*cItr == obj)
		{
			cItr++;
			continue;
		}
		if (obj->proxCol2((*cItr),SELBOUND))
			return 2; //Partially contains a child
		bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
		if (pn1)
			ret = 1;
		if ((*cItr)->isCut)
		{
			bool pn2 = obj->contains((*cItr)->x2,(*cItr)->y2);
			bool pn3 = obj->contains((*cItr)->x1,(*cItr)->y2);
			bool pn4 = obj->contains((*cItr)->x2,(*cItr)->y1);
			if (pn1 != pn2 || pn1 != pn3 || pn1 != pn4)
				return 2; //Object partially contains a child
		}
		cItr++;
	}
	//if (obj->proxCol(p1,SELBOUND))
	//	return 2;
	return ret;
}

//A Copy, Move, or Resize operation failed.
//Reset objects and provide an error message.
void fail(int code)
{
	switch (code)
	{
	case 0:
		break;
	case 1:
		ttip.setText("Not contained entirely by one subtree.");
		break;
	case 2:
		ttip.setText("Object cannot be removed from current location.");
		break;
	case 3:
		ttip.setText("Cut intersects current children.");
		break;
	case 4:
		ttip.setText("Cut intersects new children.");
		break;
	case 5:
		ttip.setText("Cannot modify old objects when Inserting.");
		break;
	case 6:
		ttip.setText("Can only Insert into selected cuts.");
		break;
	default:
		ttip.setText("Operation is invalid.");
		break;
	}
	ttip.setColor(1.0,0.0,0.0);
	graphNode* obj = alterObj.second;
	selectObj* sObj = alterObj.first;
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

void abort(int code)
{
	switch (code)
	{
	case 0:
		break;
	case 1:
		ttip.setText("Not contained entirely by one subtree.");
		break;
	case 2:
		ttip.setText("Cut is too small.");
		break;
	case 3:
		ttip.setText("Cut does not fully contain new children.");
		break;
	case 4:
		ttip.setText("Cut intersects new children.");
		break;
	case 5:
		ttip.setText("Cannot modify old children when Inserting.");
		break;
	case 6:
		ttip.setText("May only Insert into selected cuts.");
		break;
	default:
		ttip.setText("Operation is invalid.");
		break;
	}
	ttip.setColor(1.0,0.0,0.0);
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
	delete nNode;	//Too small of a cut. Abort.
	nNode = 0;
	return;
}

//Mouse button is pressed or released
void Mouse(int button, int state, int x, int y)
{
	int mod = glutGetModifiers();

	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON)
	{
		//Copy, Move, or Resize operation
		if (alterObj.second != 0)
		{
			graphNode* obj = alterObj.second;
			selectObj* sObj = alterObj.first;
			int type = alterObj.first->type;
			graphNode* parent = colParent(obj);
			if (parent == 0)
			{
				fail(1);
				return;
			}
			bool newParent = parent != obj->parent;
			if (premiseMode == false)
			{
				//Copy or Move
				std::string inf = "none";
				if (type == 3 || (type == 1 && newParent))
				{
					if (verifyStep("Iteration"))
						inf = "Iteration";
					else if(verifyStep("InsertionC"))
						inf = "InsertionC";
					else
					{
						fail(0);
						return;
					}
				}

				if (type == 1 && newParent)
				{
					//Adding these is valid. Check to see if the
					//old ones can be removed.
					if (obj->getDepth() % 2 == 0 || obj->duplicates(obj) != 0)
					{
						applyStep(inf);
						sObj->set(0,0,0,0,0,0,0);
						alterObj.second = 0;
					}
					else
						fail(2);
				}
				else if (type == 3)
				{
					applyStep(inf);
					sObj->set(0,0,0,0,0,0,0);
					alterObj.second = 0;
				}
				return;
			}
			else
			{
				int childChange = colChildren(obj);
				if (childChange == 2)
				{
					fail(3);
					return;
				}
				int childSteal = colSteal(obj, parent);
				if (childSteal == 2)
					fail(4);
				if (insertionMode && (childChange == 1 || childSteal == 1 || newParent))
				{
					/* Insertion mode stuff
					   The structure of the graph has changed, so run additional checks:
						Check that the altered object is not an old object
						Check that parent is either a selected cut or not an old object
						Check that nothing being 'stolen' is an old object
					*/
					if (insertInArea(parent) == false || 
						(insertIsArea(parent) == false && insertIsOld(parent)))
					{
						fail(6);
						return;
					}
					else if (insertIsOld(obj) || insertInArea(obj) == false)
					{
						fail(5);
						return;
					}
					std::list<graphNode*>::iterator cItr = parent->children.begin();
					while (cItr != parent->children.end())
					{
						bool pn1 = obj->contains((*cItr)->x1,(*cItr)->y1);
						if (insertIsOld((*cItr)) && pn1)
						{
							fail(5);
							return;
						}
						cItr++;
					}
				}
				if (childChange == 1)
				{
					std::list<graphNode*>::iterator cItr = obj->children.begin();
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
				if (childSteal == 1)
				{
					obj->stealChildren(parent); //We've already checked for the failure cases.

					//Remove any children stolen by this object from former parent.
					std::list<graphNode*>::iterator cItr = obj->children.begin();
					while (cItr != obj->children.end())
					{
						obj->parent->children.remove((*cItr));
						cItr++;
					}
				}
				if (newParent && type == 1)
				{
					obj->parent->children.remove(obj);
					(*displayItr)->add(parent,obj);
				}
				else if (type == 3) //Copied object only thinks it has the right parent
				{
					(*displayItr)->add(parent,obj);
					obj->setColor(0.0,0.0,0.0);
				}
				sObj->set(0,0,0,0,0,0,0);
				alterObj.second = 0;
			}
		}
		else if (premiseMode == true && nNode != 0)
		{
			std::pair<float,float> mLoc = getLoc(x,y);
			if (abs(mLoc.first - nNode->x1) > 0.15 || abs(mLoc.second - nNode->y1) > 0.15)
			{
				delete nNode;
				nNode = 0;
				return;
			}
			graphNode* p1 = (*displayItr)->getLowestSubtree(nNode->x1, nNode->y1);
			if (nNode->proxCol(p1,SELBOUND))
			{
				delete nNode;
				nNode = 0;
				return;
			}
			else if (insertionMode)
			{
				if (insertInArea(p1) == false || 
					(insertIsArea(p1) == false && insertIsOld(p1)))
				{
					delete nNode;
					nNode = 0;
					return;
				}
			}
			takeInTxt = true;
			ttip.setText("Enter object label:");
			if (insertionMode)
				ttip.setColor(0.3,0.7,0.9);
			else
				ttip.setColor(0.9,0.1,0.7);
			glutSetWindow(tooltipWindow);
			glutPostRedisplay();
		}
		return;
	}

	else if (state == GLUT_UP && button == GLUT_RIGHT_BUTTON)
	{
		if (premiseMode == false && nNode != 0)
		{
			if (verifyStep("DoubleCutAdd"))
				applyStep("DoubleCutAdd");
			else
				delete nNode;
			nNode = 0;
		}
		else if (premiseMode)
		{
			if (nNode != 0)
			{
				if (abs(nNode->x1 - nNode->x2) < 0.2 || abs(nNode->y1 - nNode->y2) < 0.2)
				{
					abort(2);
					return;
				}
				graphNode* parent = colParent(nNode);
				if (parent == 0)
				{
					abort(1);
					return;
				}
				else if (nNode->proxCol(parent,SELBOUND))
				{
					abort(4);
					return;
				}
				else if (insertionMode)
				{
					if (insertInArea(parent) == false || 
						(insertIsArea(parent) == false && insertIsOld(parent)))
					{
						abort(6);
						return;
					}
					int stealChild = colSteal(nNode,parent);
					if (stealChild == 2)
					{
						abort(3);
						return;
					}
					else if (stealChild == 1)
					{
						std::list<graphNode*>::iterator cItr = parent->children.begin();
						while (cItr != parent->children.end())
						{
							bool pn1 = nNode->contains((*cItr)->x1,(*cItr)->y1);
							if (pn1 && insertIsOld(*cItr))
							{
								abort(5);
								return;
							}
							cItr++;
						}
					}
				}
				
				if (nNode->stealChildren(parent))
				{
					nNode->setColor(0.0,0.0,0.0);
					(*displayItr)->insert(parent,nNode);
				}
				else
					abort(3);
				nNode = 0;
			}
		}
		return;
	}

	else if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON)
	{
		std::pair<float,float> mLoc = getLoc(x,y);
		graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
		if (sPick != (*displayItr)->root) //Clicked on something
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
			alterObj.second = 0; //Reset object for alteration, just in case.
		}
		else if (premiseMode)
		{
			nNode = new graphNode();
			nNode->x1 = mLoc.first;
			nNode->y1 = mLoc.second;
		}
		return;
	}

	else if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON)
	{
		std::pair<float,float> mLoc = getLoc(x,y);
		graphNode* sPick = (*displayItr)->getObject(mLoc.first, mLoc.second);
		if (sPick != (*displayItr)->root) //Clicked on something
		{
			int i = (*displayItr)->isSelected(sPick);
			if (i == 1)
				(*displayItr)->deselect(sPick,0.0,0.0,0.0);
			else if (i == 2)
				(*displayItr)->deselectSubtree(sPick,0.0,0.0,0.0);
		}
		else
		{
			nNode = new graphNode();
			nNode->isCut = true;
			nNode->setColor(0.5,0.5,0.5);
			nNode->x1 = mLoc.first;
			nNode->y1 = mLoc.second;
			nNode->x2 = mLoc.first;
			nNode->y2 = mLoc.second;

			if (premiseMode == false)
			{
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
		return;
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
	//Scroll the screen if you get close to the edges
	if (x < (float)screenWidth/10 && x >= 0)
		cameraVX = -0.2;
	else if (x > (float)screenWidth*9/10 && x <= screenWidth)
		cameraVX = 0.2;
	else
		cameraVX = 0.0;
	if (y < (float)screenHeight/10 && y >= 0)
		cameraVY = 0.2;
	else if (y > (float)screenHeight*9/10 && y <= screenWidth)
		cameraVY = -0.2;
	else
		cameraVY = 0.0;
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

/*std::pair<float,float> getLoc(int mx, int my)
{
	float h = tan((float)45/2);
	float w = h*(float)screenWidth/(float)screenHeight;

	float dH = (h - my)/(float)h;
	float dW = (w - mx)/(float)w;

	float y = cameraY + cameraZ*dH;
	float x = cameraX - cameraZ*dW;

	std::pair<float,float> loc;
	loc.first = x;
	loc.second = y;
	return loc;
}
*/
void animate(int value)
{
	cameraX+= cameraVX;
	cameraY+= cameraVY;
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
	takeInTxt = false;
	insertionMode = false;
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
	else if (rule.compare("Insertion") == 0)
	{
		//Determine if everything selected is a cut at an even level.
		while (itr != (*displayItr)->selection.end())
		{
			if ((*itr).first->isCut == false)
			{
				ttip.setText("At least one selected object is not a cut.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			if ((*itr).first->getDepth() % 2 == 1)
			{
				ttip.setText("At least one selected area is not an odd level.");
				ttip.setColor(1.0,0.0,0.0);
				verify = false;
				break;
			}
			itr++;
		}
	}
	else if (rule.compare("Iteration") == 0)
		verify = verifyIteration();
	else if (rule.compare("InsertionC") == 0)
		verify = verifyInsertionC();
	else if (rule.compare("DoubleCutAdd") == 0)
		verify = verifyDCutAdd();
	else
	{
		ttip.setText("Unknown inference rule");
		ttip.setColor(1.0,0.0,0.0);
	}
	glutSetWindow(tooltipWindow);
	glutPostRedisplay();
	return verify;
}

bool verifyIteration()
{
	graphNode* obj = alterObj.second;
	selectObj* sObj = alterObj.first;
	//Boundary checking
	graphNode* p1 = colParent(obj);
	if (p1 == 0)
	{
		ttip.setText("Not contained entirely by one subtree.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	else if (obj->proxCol(p1,SELBOUND))
	{
		//Too close to something else. Fail.
		ttip.setText("Object or child is too close to another object.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	if (p1 != obj->parent)
	{
		//Object has a new parent. Check for validity.
		if (!(p1->inSubtree(obj->parent)))
		{
			ttip.setText("Object can only be iterated to a lower level in the same subtree.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
	}
	if (obj->isCut)
	{
		int colChild = colChildren(obj);
		if (colChild == 2)
		{
			ttip.setText("A child would be partially contained by different subtrees.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
		else if (colChild == 1)
		{
			ttip.setText("All children must remain inside this cut.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
		//Check if it now contains new objects.
		int stealChild = colSteal(obj, p1);
		if (stealChild == 2)
		{
			ttip.setText("Cut would partially contain another cut.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
		else if (stealChild == 1)
		{
			ttip.setText("Cut cannot steal existing children.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
	}
	return true;
}

bool verifyInsertionC()
{
	//Special case of Insertion where the object to be
	//added already exists and is being moved or copied
	//to the location rather than drawn.
	graphNode* obj = alterObj.second;
	selectObj* sObj = alterObj.first;
	graphNode* p1 = colParent(obj);
	if (p1 == 0)
	{
		ttip.setText("Not contained entirely by one subtree.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	else if (p1->getDepth() % 2 != 0)
	{
		ttip.setText("Object can only be Inserted at an odd level.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	else if (obj->proxCol(p1,SELBOUND))
	{
		//Too close to something else. Fail.
		ttip.setText("Object or child is too close to another object.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	if (obj->isCut)
	{
		int colChild = colChildren(obj);
		if (colChild == 2)
		{
			ttip.setText("A child would be partially contained by different subtrees.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
		//Check if it now contains new objects.
		int stealChild = colSteal(obj, p1);
		if (stealChild == 2)
		{
			ttip.setText("Cut would partially contain another object.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
		else if (stealChild == 1)
		{
			ttip.setText("Cut cannot steal existing children.");
			ttip.setColor(1.0,0.0,0.0);
			return false;
		}
	}
	return true;
}
bool verifyDCutAdd()
{
	graphNode* nn2 = nNode->children.front();
	if (abs(nn2->x1 - nn2->x2) < SELBOUND*4 || abs(nn2->y1 - nn2->y2) < SELBOUND*4)
	{
		ttip.setText("Cut is too small.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
	graphNode* p1 = colParent(nNode);
	if (p1 == 0)
	{
		ttip.setText("Cut is not contained entirely by one subtree.");
		ttip.setColor(1.0,0.0,0.0);
		return false;
	}
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
				return false;
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
					return false;
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
				return false;
			}
		}
		cItr++;
	}
	return true;
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
	else if (rule.compare("Insertion") == 0)
	{
		std::list<std::pair<graphNode*,bool> >::iterator itr = (*displayItr)->selection.begin();
		std::list<graphNode*>::iterator cItr;
		while (itr != (*displayItr)->selection.end())
		{
			insertionCuts.push_back((*itr).first);
			cItr = (*itr).first->children.begin();
			while (cItr != (*itr).first->children.end())
			{
				insertionChildren.push_back(*cItr);
				cItr++;
			}
			itr++;
		}
		insertionMode = true;
		premiseMode = true;
		ttip.setText("Insertion Mode.");
		ttip.setColor(0.3,0.7,0.9);
		glutSetWindow(tooltipWindow);
		glutPostRedisplay();
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

bool insertInArea(graphNode* obj)
{
	std::list<graphNode*>::iterator itr = insertionCuts.begin();
	while (itr != insertionCuts.end())
	{
		if (obj->inSubtree(*itr) || obj == *itr)
			return true;
		itr++;
	}
	return false;
}
bool insertIsArea(graphNode* obj)
{
	std::list<graphNode*>::iterator itr = insertionCuts.begin();
	while (itr != insertionCuts.end())
	{
		if (obj == *itr)
			return true;
		itr++;
	}
	return false;
}
bool insertIsOld(graphNode* obj)
{
	if (insertIsArea(obj))
		return true;
	std::list<graphNode*>::iterator itr = insertionChildren.begin();
	while (itr != insertionChildren.end())
	{
		if (obj == *itr || obj->inSubtree(*itr))
			return true;
		itr++;
	}
	return false;
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