#include <GL/glew.h>
#include "FrameBufferObject.h"

#include "ShaderObject.h"
#include "ShaderProgram.h"
#include "ShaderUniformValue.h"
#include "GlowEffect.h"

GlowEffect::GlowEffect(unsigned int width, unsigned int height)
: imageWinWidth( width )
, imageWinHeight( height )
{
}

GlowEffect::~GlowEffect(void)
{
}

void GlowEffect::begin()
{
	// ******* RENDER TO THE FRAMEBUFFER ********
	//bind the FBO, and the associated texture.
	fbo->bind();

	// #### FIRST STEP: Draw the object into the original texture: 'originalTexture'
	glClearColor(0.0f, 0.0f, 0.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );

	/************************************************************************/
	/* ######################### DRAW HERE!!!!                              */
	/************************************************************************/
}

void GlowEffect::end()
{
	// #### SECOND STEP: blur the texture horizontally 
	// and store it into the GL_COLOR_ATTACHMENT1_EXT or  'horizBlurredTex'
	// Render it to the right texture: 'horizBlurredTex'
	glDrawBuffer( GL_COLOR_ATTACHMENT1_EXT );

	// render using the horizontal blur shader and the original texture
	shaderProgram->useProgram();
	renderSceneOnQuad( originalTexture, GL_TEXTURE0);
	shaderProgram->disableProgram();

	// #### THIRD STEP: blur the texture vertically 
	// and store it into the 'finalBlurredTex'
	// Choose the right attachment
	glDrawBuffer( GL_COLOR_ATTACHMENT2_EXT );

	// render using the vertical blur shader and the horizontal blurred texture
	shaderProgram2->useProgram();
	renderSceneOnQuad( horizBlurredTex, GL_TEXTURE0 );
	shaderProgram2->disableProgram();

	// 'unbind' the FBO. things will now be drawn to screen as usual
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// ******* RENDER TO THE WINDOW *************

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/************************************************************************/
	/* SHOULD FIND OUT THIS WELL !!!!                                       */
	/************************************************************************/
	glColor3f(1.0, 1.0, 1.0);

	shaderProgram->useProgram();
	renderSceneOnQuad( finalBlurredTex, GL_TEXTURE0 );
	shaderProgram->disableProgram();
}

void GlowEffect::renderSceneOnQuad(GLuint textureId, GLenum textureUnit)
{
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D( -1.0f, 1.0f, -1.0f, 1.0f );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	glEnable( GL_TEXTURE_2D );

	glActiveTexture( textureUnit );
	glBindTexture(GL_TEXTURE_2D, textureId);

	//RENDER MULTITEXTURE ON QUAD
	glBegin(GL_QUADS);
	glMultiTexCoord2f( textureUnit, 0.0f, 0.0f );
	glVertex2f( -1.0f, -1.0f );

	glMultiTexCoord2f( textureUnit, 1.0f, 0.0f );
	glVertex2f( 1.0f, -1.0f );

	glMultiTexCoord2f( textureUnit, 1.0f, 1.0f );
	glVertex2f( 1.0f, 1.0f );

	glMultiTexCoord2f( textureUnit, 0.0f, 1.0f );
	glVertex2f( -1.0f, 1.0f );
	glEnd();

	glPopMatrix();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

/*
* Initializes the program shaders	
*/
void GlowEffect::initShaders()
{
	horizontalBlurVertexShader = new ShaderObject(GL_VERTEX_SHADER, "./shaders/horizBlur.vert");
	horizontalBlurFragmentShader = new ShaderObject(GL_FRAGMENT_SHADER, "./shaders/horizBlur.frag");

	verticalBlurVertexShader = new ShaderObject(GL_VERTEX_SHADER, "./shaders/vertBlur.vert");
	verticalBlurFragmentShader = new ShaderObject(GL_FRAGMENT_SHADER, "./shaders/vertBlur.frag");

	shaderProgram = new ShaderProgram();

	shaderProgram2 = new ShaderProgram();

	shaderProgram->attachShader( *horizontalBlurVertexShader );
	shaderProgram->attachShader( *horizontalBlurFragmentShader );

	shaderProgram2->attachShader( *verticalBlurFragmentShader );
	shaderProgram2->attachShader( *verticalBlurVertexShader );

	textureUniform.setValue( 0 );
	textureUniform.setName("blurTex");

	shaderProgram->addUniformObject( &textureUniform );
	shaderProgram2->addUniformObject( &textureUniform );

	shaderProgram->buildProgram();
	shaderProgram2->buildProgram();
}

/**
*/
void GlowEffect::init()
{
	// ########### init shaders ###############
	initShaders();

	fbo = new FrameBufferObject();

	/************************************************************************/
	/* TEXTURE PART                                                         */
	/************************************************************************/
	// make a texture
	glGenTextures(1, &originalTexture);
	glGenTextures(1, &horizBlurredTex);
	glGenTextures(1, &finalBlurredTex);

	fbo->bind();

	// initialize texture that will store the framebuffer image
	glBindTexture(GL_TEXTURE_2D, originalTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWinWidth, imageWinHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, horizBlurredTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWinWidth, imageWinHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, finalBlurredTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWinWidth, imageWinHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	fbo->attachTexture(originalTexture, GL_COLOR_ATTACHMENT0_EXT);
	fbo->attachTexture(horizBlurredTex, GL_COLOR_ATTACHMENT1_EXT);
	fbo->attachTexture(finalBlurredTex, GL_COLOR_ATTACHMENT2_EXT);

	FrameBufferObject::unbind();
}