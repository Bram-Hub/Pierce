#ifndef tooltip_h_
#define tooltip_h_

#include <string>
#include <iostream>

class toolTip
{
public:
	toolTip() {};
	toolTip(float r, float g, float b);
	~toolTip() {};
	void setText(char* txt) { text.assign(txt); };
	void setText(std::string txt) { text = txt; };
	void setColor(float r, float g, float b);
	void draw(int w, int h);
	float mx, my;
private:
	std::string text;
	float mR, mG, mB;
};

toolTip::toolTip(float r, float g, float b)
{
	mR = r;
	mG = g;
	mB = b;
	mx = 0;
	my = 0;
}

void toolTip::setColor(float r, float g, float b)
{
	mR = r;
	mG = g;
	mB = b;
}

void toolTip::draw(int w, int h)
{
	GLfloat vScale = 0.75*tan((float)45/2)*7;
	GLfloat hScale = (float)w/(float)h * vScale;

	glColor4f(mR,mG,mB,1.0f);
	glRasterPos2f(2-hScale,-vScale*3/8);
	//glRasterPos2f(0,0);
	for (unsigned int i = 0; i < text.length(); i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text.c_str()[i]);
	}
}

#endif