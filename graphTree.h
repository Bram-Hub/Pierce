#ifndef graphtree_h_
#define graphtree_h_

#include <list>

class graphNode
{
public:
	graphNode();
	graphNode(graphNode* old);
	~graphNode();

	void addChild(graphNode* child) { children.push_back(child); };
	void draw();
	void setLabel(char* txt);
	void setColor(float r, float g, float b);

	bool contains(float x, float y);
	graphNode* childContains(float x, float y);
	bool stealChildren(graphNode* parent);
	graphNode* getObject(float x, float y);
	bool inSubtreeOf(graphNode* root);
	bool inSubtree(graphNode* root);
	bool eqv(graphNode* old);
	bool eqo(graphNode* old);
	bool lblEqv(char* lblOld);

	void moveAll(float x, float y);
	void move(float x, float y);
	bool proxCol(graphNode* parent, float SELBOUND);
	graphNode* duplicates(graphNode* obj);

	int getDepth();

	//void selectSubtree(graphTree* tree, float r1, float g1, float b1, float r2, float g2, float b2);
	//void deselectSubtree(float r, float g, float b);

	void debugPrint(unsigned int d);

	bool listContains(std::list<graphNode*> lst, graphNode* obj);

	graphNode* parent;
	std::list<graphNode*> children;

	//Object properties
	float x1,y1, x2,y2; //Location
	float mR,mG,mB;		//Color
	bool isCut;			//Type
	char label[128];	//Label
	unsigned int lblLen;
};

graphNode::graphNode()
{
	parent = 0;

	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
	mR = 0;
	mG = 0;
	mB = 0;
	isCut = false;
	label[0] = 0;
	lblLen = 0;
}

graphNode::graphNode(graphNode* old)
{
	parent = old->parent;
	x1 = old->x1;
	y1 = old->y1;
	x2 = old->x2;
	y2 = old->y2;
	mR = old->mR;
	mG = old->mG;
	mB = old->mB;
	isCut = old->isCut;
	setLabel(old->label);
	lblLen = old->lblLen;

	graphNode* nNode;
	std::list<graphNode*>::iterator itr = old->children.begin();
	while (itr != old->children.end())
	{
		nNode = new graphNode((*itr));
		nNode->parent = this;
		children.push_back(nNode);
		itr++;
	}
}

graphNode::~graphNode()
{
	std::list<graphNode*>::iterator itr = children.begin();
	while (itr != children.end())
	{
		delete (*itr);
		itr++;
	}
}

void graphNode::draw()
{
	std::list<graphNode*>::iterator itr = children.begin();
	while (itr != children.end())
	{
		(*itr)->draw();
		itr++;
	}
	//Insert drawing routines for object properties
	if (isCut)
	{
		glColor3f(mR, mG, mB);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2.0f);
		glRectf(x1,y1, x2,y2);
	}
	else
	{
		glColor3f(mR, mG, mB);
		glRasterPos2f(x1-0.1,y1-0.1);
		for (unsigned int i = 0; i < lblLen; i++)
		{
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, label[i]);
		}
	}
}

bool graphNode::contains(float x, float y)
{
	if (!isCut) return false;
	if ((x > x1 && x < x2) || (x > x2 && x < x1))
	{
		if ((y > y1 && y < y2) || (y > y2 && y < y1))
			return true;
	}
	return false;
}
/*bool graphNode::contains(float x, float y)
{
	if (!isCut) return false;
	if ((x >= x1 && x <= x2) || (x >= x2 && x <= x1))
	{
		if ((y >= y1 && y <= y2) || (y >= y2 && y <= y1))
			return true;
	}
	return false;
}*/

graphNode* graphNode::childContains(float x, float y)
{
	//if (!(this->contains(x,y))) return (graphNode*)0;
	std::list<graphNode*>::iterator itr = children.begin();
	while (itr != children.end())
	{
		if ((*itr)->contains(x,y))	//At most one child can contain this point
			return (*itr)->childContains(x,y);
		itr++;
	}
	return this;
}

//Finds all children of the given parent that are contained
//within itself. Assumes that it is itself contained entirely
//within the given parent.
//Will add these children to its own list of children.
//Returns false if there are children found that are partially
//contained within itself, true otherwise.
bool graphNode::stealChildren(graphNode* parent)
{
	std::list<graphNode*>::iterator itr = parent->children.begin();
	while (itr != parent->children.end())
	{
		if ((*itr) == this)
		{
			itr++;
			continue;
		}
		if ((*itr)->isCut)
		{
			int corners = 0;
			if (this->contains((*itr)->x1,(*itr)->y1))
				corners++;
			if (this->contains((*itr)->x2,(*itr)->y2))
				corners++;
			if (this->contains((*itr)->x1,(*itr)->y2))
				corners++;
			if (this->contains((*itr)->x2,(*itr)->y1))
				corners++;
			if (corners == 4)
			{
				children.push_back((*itr));
				(*itr)->parent = this;
			}
			else if (corners > 0)
				return false;
		}
		else
		{
			if (this->contains((*itr)->x1,(*itr)->y1))
			{
				children.push_back((*itr));
				(*itr)->parent = this;
			}
		}
		itr++;
	}
	return true;
}

graphNode* graphNode::getObject(float x, float y)
{
	if (isCut)
	{
		if (abs(x-x1) < 0.1 || abs(x-x2) < 0.1)
		{
			if ((y <= y1-0.1 && y >= y2+0.1) || (y <= y2-0.1 && y >= y1+0.1))
				return this;
		}
		else if (abs(y-y1) < 0.1 || abs(y-y2) < 0.1)
		{
			if ((x <= x1-0.1 && x >= x2+0.1) || (x <= x2-0.1 && x >= x1+0.1))
				return this;
		}
		else
		{
			graphNode* obj;
			std::list<graphNode*>::iterator itr = children.begin();
			while (itr != children.end())
			{
				obj = (*itr)->getObject(x,y);
				if (obj != 0)
					return obj;
				itr++;
			}
		}
		return (graphNode*) 0;
	}
	else
	{
		if (abs(x-x1) < 0.2 && abs(y-y1) < 0.2)
			return this;
		else
		{
			graphNode* obj;
			std::list<graphNode*>::iterator itr = children.begin();
			while (itr != children.end())
			{
				obj = (*itr)->getObject(x,y);
				if (obj != 0)
					return obj;
				itr++;
			}
		}
		return (graphNode*) 0;
	}
}

bool graphNode::inSubtreeOf(graphNode* root)
{
	if (root->parent == 0 || parent == root || parent == root->parent)
		return true;
	if (parent == 0)
		return false;
	return parent->inSubtreeOf(root);
}
bool graphNode::inSubtree(graphNode* root)
{
	if (root == 0 || parent == root)
		return true;
	if (parent == 0)
		return false;
	return parent->inSubtree(root);
}

//Equivalence of two graphNodes.
//They are 'equal' if they have the same x,y coordinate pairs, the same type,
//and the same label.
bool graphNode::eqv(graphNode* old)
{
	if (isCut && old->isCut)
	{
		if (x1 == old->x1 && y1 == old->y1 && x2 == old->x2 && y2 == old->y2)
			return true;
	}
	else if (!isCut && !(old->isCut))
	{
		if (x1 == old->x1 && y1 == old->y1 && lblEqv(old->label))
			return true;
	}
	return false;
}

//Equivalence of two graphNodes.
//They are 'equal' if they have the same type and label, and if all of their
//children are similarly equivalent.
bool graphNode::eqo(graphNode* old)
{
	if (isCut != old->isCut || children.size() != old->children.size() || !lblEqv(old->label))
		return false;
	std::list<graphNode*>::iterator itr = children.begin();
	std::list<graphNode*>::iterator oItr;
	std::list<graphNode*> used;
	while (itr != children.begin())
	{
		oItr = old->children.begin();
		while (oItr != old->children.end())
		{
			if (!listContains(used,(*oItr)) && (*itr)->eqo((*oItr)))
			{
				used.push_back((*oItr));
				break;
			}
			oItr++;
		}
		if (oItr == old->children.end())
			return false;
		itr++;
	}
	return true;
}

//Equivalence of two graphNode labels.
bool graphNode::lblEqv(char* lblOld)
{
	if (isCut) return true;
	for (unsigned int i = 0; i < 128; i++)
	{
		if (label[i] != lblOld[i])
			return false;
	}
	return true;
}

void graphNode::setLabel(char* txt)
{
	for (unsigned int i = 0; i < 128; i++)
		label[i] = txt[i];
}

void graphNode::setColor(float r, float g, float b)
{
	mR = r;
	mG = g;
	mB = b;
}

int graphNode::getDepth()
{
	if (parent == 0)
		return -1;
	else
		return (parent->getDepth())+1;
}

void graphNode::debugPrint(unsigned int d)
{
	std::cout << d;
	for (unsigned int i = 0; i < d; i++)
	{
		std::cout << " ";
	}
	if (isCut)
		std::cout << "-\n";
	else
	{
		for (unsigned int i = 0; i < lblLen; i++)
			std::cout << label[i];
		std::cout << "\n";
	}

	++d;
	std::list<graphNode*>::iterator itr = children.begin();
	while (itr != children.end())
	{
		(*itr)->debugPrint(d);
		itr++;
	}
}

bool graphNode::listContains(std::list<graphNode*> lst, graphNode* obj)
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

//Move the object so that (x1,y1) is at (x,y).
//Also move its children so the relative position
//to their parent is the same as before.
void graphNode::moveAll(float x, float y)
{
	float difX = x-x1;
	float difY = y-y1;
	x1+= difX;
	y1+= difY;
	if (isCut)
	{
		x2+= difX;
		y2+= difY;
	}

	std::list<graphNode*>::iterator itr = children.begin();
	while (itr != children.end())
	{
		(*itr)->moveAll((*itr)->x1+difX,(*itr)->y1+difY);
		itr++;
	}
}
//Move the object as in moveAll, but doesn't
//move the children as well.
void graphNode::move(float x, float y)
{
	float difX = x-x1;
	float difY = y-y1;
	x1+= difX;
	y1+= difY;
	if (isCut)
	{
		x2+= difX;
		y2+= difY;
	}
}
//Checks if there is a "collision" -- ie, this object is
//too close to another child of the specified parent.
//It will also check to see if any of its children (and
//its children's children, etc.) collide.
bool graphNode::proxCol(graphNode* parent, float bound)
{
	return false;
	//Check for collisions against the parent, if parent isn't the root.
	if (parent->isCut)
	{
		if (this->isCut)
		{
			if (abs(parent->x1 - x1) < bound || abs(parent->x2 - x1) < bound ||
				abs(parent->x1 - x2) < bound || abs(parent->x2 - x2) < bound)
			{
				if (!( (parent->y1+bound < y1 && parent->y1+bound < y2 &&
						parent->y2+bound < y1 && parent->y2+bound < y2) ||
						(parent->y1-bound > y1 && parent->y1-bound > y2 &&
						parent->y2-bound > y1 && parent->y2-bound > y2)))
					return true;
			}
			else if (abs(parent->y1 - y1) < bound || abs(parent->y2 - y1) < bound ||
				abs(parent->y1 - y2) < bound || abs(parent->y2 - y2) < bound)
			{
				if (!( (parent->x1+bound < x1 && parent->x1+bound < x2 &&
						parent->x2+bound < x1 && parent->x2+bound < x2) ||
						(parent->x1-bound > x1 && parent->x1-bound > x2 &&
						parent->x2-bound > x1 && parent->x2-bound > x2)))
					return true;
			}
		}
		else
		{
			if (abs(parent->x1 - x1) < bound || abs(parent->x2 - x1) < bound)
			{
				if ((parent->y1+bound < y1 && parent->y2-bound > y1) ||
					(parent->y2+bound < y1 && parent->y1-bound > y1))
					return true;
			}
			else if (abs(parent->y1 - y1) < bound || abs(parent->y2 - y1) < bound)
			{
				if ((parent->x1+bound < x1 && parent->x2-bound > x1) ||
					(parent->x2+bound < x1 && parent->x1-bound > x1))
					return true;
			}
		}
	}
	std::list<graphNode*>::iterator itr = parent->children.begin();
	while (itr != parent->children.end())
	{
		if ((*itr) == this) //Skip itself.
		{
			itr++;
			continue;
		}
		if ((*itr)->isCut)
		{
			if (this->isCut)
			{
				if (abs((*itr)->x1 - x1) < bound || abs((*itr)->x2 - x1) < bound ||
					abs((*itr)->x1 - x2) < bound || abs((*itr)->x2 - x2) < bound)
				{
					if (!( ((*itr)->y1+bound < y1 && (*itr)->y1+bound < y2 &&
							(*itr)->y2+bound < y1 && (*itr)->y2+bound < y2) ||
							((*itr)->y1-bound > y1 && (*itr)->y1-bound > y2 &&
							(*itr)->y2-bound > y1 && (*itr)->y2-bound > y2)))
						return true;
				}
				else if (abs((*itr)->y1 - y1) < bound || abs((*itr)->y2 - y1) < bound ||
					abs((*itr)->y1 - y2) < bound || abs((*itr)->y2 - y2) < bound)
				{
					if (!( ((*itr)->x1+bound < x1 && (*itr)->x1+bound < x2 &&
							(*itr)->x2+bound < x1 && (*itr)->x2+bound < x2) ||
							((*itr)->x1-bound > x1 && (*itr)->x1-bound > x2 &&
							(*itr)->x2-bound > x1 && (*itr)->x2-bound > x2)))
						return true;
				}
			}
			else
			{
				if (abs((*itr)->x1 - x1) < bound || abs((*itr)->x2 - x1) < bound)
				{
					if (((*itr)->y1+bound < y1 && (*itr)->y2-bound > y1) ||
						((*itr)->y2+bound < y1 && (*itr)->y1-bound > y1))
						return true;
				}
				else if (abs((*itr)->y1 - y1) < bound || abs((*itr)->y2 - y1) < bound)
				{
					if (((*itr)->x1+bound < x1 && (*itr)->x2-bound > x1) ||
						((*itr)->x2+bound < x1 && (*itr)->x1-bound > x1))
						return true;
				}
			}
		}
		else
		{
			if (this->isCut)
			{
				if (abs((*itr)->x1 - x1) < bound || abs((*itr)->x1 - x2) < bound)
				{
					if (((*itr)->y1 < y1-bound && (*itr)->y1 > y2+bound) ||
						((*itr)->y1 < y2-bound && (*itr)->y1 > y1+bound))
						return true;
				}
				else if (abs((*itr)->y1 - y1) < bound || abs((*itr)->y1 - y2) < bound)
				{
					if (((*itr)->x1 < x1-bound && (*itr)->x1 > x2+bound) ||
						((*itr)->x1 < x2-bound && (*itr)->x1 > x1+bound))
						return true;
				}
			}
			else
			{
				if (abs((*itr)->x1 - x1) < bound || abs((*itr)->y1 - y1) < bound)
					return true;
			}
		}
		itr++;
	}
	itr = this->children.begin();
	while (itr != this->children.end())
	{
		if ((*itr)->proxCol(parent,bound))
			return true;
		itr++;
	}
	return false;
}

//Determine if there is a duplicate of the given node
//at an equal or lower depth in this subtree.
//Doesn't count itself for this.
graphNode* graphNode::duplicates(graphNode* obj)
{
	if (parent == 0)
		return 0;
	std::list<graphNode*>::iterator itr = parent->children.begin();
	while (itr != parent->children.end())
	{
		if ((*itr) != this && (*itr)->eqo(obj) == true)
			return (*itr);
		itr++;
	}
	return parent->duplicates(obj);
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//graphTree class

class graphTree
{
public:
	graphTree() { root = new graphNode(); root->parent = 0; };
	graphTree(graphTree* old);
	~graphTree() { delete root; };

	void add(graphNode* parent, graphNode* newNode) { parent->addChild(newNode); newNode->parent = parent; };
	void insert(graphNode* parent, graphNode* newNode);
	graphNode* getLowestSubtree(float x, float y);
	graphNode* getObject(float x, float y);

	graphNode* findEqNode(graphNode* old);

	void draw() { root->draw(); };
	void select(graphNode* obj, float r, float g, float b);
	void deselect(graphNode* obj, float r, float g, float b);
	void selectSubtree(graphNode* obj, float r1, float g1, float b1, float r2, float g2, float b2);
	void selectSub(graphNode* obj, float r, float g, float b);
	void deselectSubtree(graphNode* obj, float r, float g, float b);
	void deselectSub(graphNode* obj, float r, float g, float b);
	int isSelected(graphNode* obj);

	void deleteSelections();

	void debugPrint() { root->debugPrint(0); };

	graphNode* root;
	std::list<std::pair<graphNode*,bool> > selection;

	//Graph properties
	//Selection list, inference, etc.
};

//Copy constructor
graphTree::graphTree(graphTree* old)
{
	root = new graphNode(old->root);
	std::list<std::pair<graphNode*,bool> >::iterator itr = old->selection.begin();
	while (itr != old->selection.end())
	{
		std::pair<graphNode*,bool> pr;
		pr.first = findEqNode((*itr).first);
		pr.second = (*itr).second;
		selection.push_back(pr);
		itr++;
	}
}

//Add new child to given node, but also takes any children
//of the new node that are also children of the given
//parent away from said parent.
//I.e. it inserts a new node between the parent and a
//subset of its children.
void graphTree::insert(graphNode* parent, graphNode* newNode)
{
	std::list<graphNode*>::iterator itr = newNode->children.begin();
	while (itr != newNode->children.end())
	{
		parent->children.remove(*itr);
		itr++;
	}
	parent->addChild(newNode);
	newNode->parent = parent;
}

//Get the lowest subtree (ie, cut) that contains the
//specified point within its boundary.
//Since graphNode::childContains doesn't check to see if
//itself actually contains the point (but checks the
//children and only calls it on them if they do), this can
//simply be invoked on the root and the root will be
//returned if nothing else contains it (which is desired).
graphNode* graphTree::getLowestSubtree(float x, float y)
{
	return root->childContains(x,y);
}

graphNode* graphTree::getObject(float x, float y)
{
	graphNode* obj;
	std::list<graphNode*>::iterator itr = root->children.begin();
	while (itr != root->children.end())
	{
		obj = (*itr)->getObject(x,y);
		if (obj != 0)
			return obj;
		itr++;
	}
	return root;
}

graphNode* graphTree::findEqNode(graphNode* old)
{
	if (old->parent == 0)
		return root;
	graphNode* nParent = findEqNode(old->parent);
	std::list<graphNode*>::iterator itr = nParent->children.begin();
	while (itr != nParent->children.end())
	{
		if ((*itr)->eqv(old))
			return (*itr);
		itr++;
	}
	return (graphNode*) 0;
}

void graphTree::select(graphNode* obj, float r, float g, float b)
{
	obj->setColor(r,g,b);
	std::pair<graphNode*,bool> s;
	s.first = obj;
	s.second = false;
	selection.push_back(s);
	std::cout << "* " << selection.size() << "\n";
}
void graphTree::deselect(graphNode* obj, float r, float g, float b)
{
	obj->setColor(r,g,b);
	std::pair<graphNode*,bool> s;
	s.first = obj;
	s.second = false;
	selection.remove(s);
	std::cout << "* " << selection.size() << "\n";
}
void graphTree::selectSubtree(graphNode* obj, float r1, float g1, float b1, float r2, float g2, float b2)
{
	obj->setColor(r1,g1,b1);
	std::list<graphNode*>::iterator itr = obj->children.begin();
	while (itr != obj->children.end())
	{
		selectSub((*itr),r2,g2,b2);
		itr++;
	}
	std::pair<graphNode*,bool> s;
	s.first = obj;
	s.second = true;
	selection.push_back(s);
	std::cout << "* " << selection.size() << "\n";
}
void graphTree::selectSub(graphNode* obj, float r, float g, float b)
{
	int i = isSelected(obj);
	if (i == 1)
		deselect(obj,0.0,0.0,0.0);
	else if (i == 2)
		deselectSubtree(obj,0.0,0.0,0.0);
	obj->setColor(r,g,b);
	std::list<graphNode*>::iterator itr = obj->children.begin();
	while (itr != obj->children.end())
	{
		selectSub((*itr),r,g,b);
		itr++;
	}
}
void graphTree::deselectSubtree(graphNode* obj, float r, float g, float b)
{
	deselectSub(obj,r,g,b);
	std::pair<graphNode*,bool> s;
	s.first = obj;
	s.second = true;
	selection.remove(s);
	std::cout << "* " << selection.size() << "\n";
}
void graphTree::deselectSub(graphNode* obj, float r, float g, float b)
{
	obj->setColor(r,g,b);
	std::list<graphNode*>::iterator itr = obj->children.begin();
	while (itr != obj->children.end())
	{
		deselectSub((*itr),r,g,b);
		itr++;
	}
}

int graphTree::isSelected(graphNode* obj)
{
	std::list<std::pair<graphNode*,bool> >::iterator itr = selection.begin();
	while (itr != selection.end())
	{
		if ((*itr).first == obj)
			return (*itr).second ? 2 : 1; //Return 2 if subtree is selected, 1 if not
		itr++;
	}
	return 0;
}

void graphTree::deleteSelections()
{
	std::list<std::pair<graphNode*,bool> >::iterator itr = selection.begin();
	while (itr != selection.end())
	{
		if ((*itr).second == false && (*itr).first->isCut == true)
		{
			std::list<graphNode*>::iterator sItr = (*itr).first->children.begin();
			while (sItr != (*itr).first->children.end())
			{
				(*itr).first->parent->addChild((*sItr));
				(*sItr)->parent = (*itr).first->parent;
				sItr++;
			}
			(*itr).first->children.clear();
		}
		(*itr).first->parent->children.remove((*itr).first);	//Ow
		delete (*itr).first;
		itr++;
	}
	selection.clear();
}


//////////////////////
//////////////////////
//SelectObj Class
class selectObj
{
public:
	selectObj();
	selectObj(float aX, float aY, float x1, float y1, float x2, float y2, int aType);
	void set(float aX, float aY, float x1, float y1, float x2, float y2, int aType);

	int type; //0 is NULL, 1 is move, 2 is resize, 3 is copy
	bool subtree; //Was object selected as a subtree
	float x, y; //Coordinates of initial selection
	float iX1, iY1, iX2, iY2; //Coordinates of initial object position
	graphNode* original;
};

selectObj::selectObj()
{
	x = 0;
	y = 0;
	iX1 = 0;
	iX2 = 0;
	iY1 = 0;
	iY2 = 0;
	type = 0;
	subtree = false;
	original = 0;
}

selectObj::selectObj(float aX, float aY, float x1, float y1, float x2, float y2, int aType)
{
	x = aX;
	y = aY;
	iX1 = x1;
	iY1 = y1;
	iX2 = x2;
	iY2 = y2;
	type = aType;
	subtree = false;
	original = 0;
}

void selectObj::set(float aX, float aY, float x1, float y1, float x2, float y2, int aType)
{
	x = aX;
	y = aY;
	iX1 = x1;
	iY1 = y1;
	iX2 = x2;
	iY2 = y2;
	type = aType;
	//original = org;
}

#endif
