
// Neede to get the prototypes for glGenBuffersARB() and friends
// Must be defined before glext.h is included, which is included
// deep in the QtOpenGL include (or even earlier, hence this top-posting.)
#define GL_GLEXT_PROTOTYPES

#include <QtGui>
#include <QtOpenGL>
#include "GLWidget.h"
#include "GLDrawable.h"

#include <math.h>

#ifdef OPENCV_ENABLED
#include <opencv/cv.h>
#endif

#include <QGLFramebufferObject>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#include "GLCommonShaders.h"

// function pointers for PBO Extension
// Windows needs to get function pointers from ICD OpenGL drivers,
// because opengl32.dll does not support extensions higher than v1.1.
#ifdef Q_OS_WIN32
	#if 0
		#ifndef glGenBuffersARB
			PFNGLGENBUFFERSARBPROC pglGenBuffersARB = 0;                     // VBO Name Generation Procedure
			PFNGLBINDBUFFERARBPROC pglBindBufferARB = 0;                     // VBO Bind Procedure
			PFNGLBUFFERDATAARBPROC pglBufferDataARB = 0;                     // VBO Data Loading Procedure
			PFNGLBUFFERSUBDATAARBPROC pglBufferSubDataARB = 0;               // VBO Sub Data Loading Procedure
			PFNGLDELETEBUFFERSARBPROC pglDeleteBuffersARB = 0;               // VBO Deletion Procedure
			PFNGLGETBUFFERPARAMETERIVARBPROC pglGetBufferParameterivARB = 0; // return various parameters of VBO
			PFNGLMAPBUFFERARBPROC pglMapBufferARB = 0;                       // map VBO procedure
			PFNGLUNMAPBUFFERARBPROC pglUnmapBufferARB = 0;                   // unmap VBO procedure
			#define glGenBuffersARB           pglGenBuffersARB
			#define glBindBufferARB           pglBindBufferARB
			#define glBufferDataARB           pglBufferDataARB
			#define glBufferSubDataARB        pglBufferSubDataARB
			#define glDeleteBuffersARB        pglDeleteBuffersARB
			#define glGetBufferParameterivARB pglGetBufferParameterivARB
			#define glMapBufferARB            pglMapBufferARB
			#define glUnmapBufferARB          pglUnmapBufferARB
		#endif
	#endif
#endif

GLWidgetSubview::GLWidgetSubview()
	: QObject()
	, m_colorsDirty(true)
	, m_viewTop(0)
	, m_viewLeft(0)
	, m_viewBottom(1.)
	, m_viewRight(1.)
	, m_sourceTop(-1)
	, m_sourceLeft(-1)
	, m_sourceBottom(-1)
	, m_sourceRight(-1)
	, m_flipHorizontal(false)
	, m_flipVertical(false)
	, m_brightness(0)
	, m_contrast(0)
	, m_hue(0)
	, m_saturation(0)
	, m_cornerRotation(GLRotateNone)
	, m_glw(0)
	, m_id(-1)
{
	m_cornerTranslations
		<<  QPointF(0,0)
		<<  QPointF(0,0)
		<<  QPointF(0,0)
		<<  QPointF(0,0);
};

int GLWidgetSubview::subviewId()
{
	if(m_id<0)
	{
		QSettings s;
		m_id = s.value("GLWidgetSubview/subview-id-counter",100).toInt() + 1;
		s.setValue("GLWidgetSubview/subview-id-counter", m_id);
	}
	return m_id;
}

void GLWidgetSubview::setGLWidget(GLWidget *glw)
{
	m_glw = glw;
}


QByteArray GLWidgetSubview::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["title"]	= m_title;
	map["id"]	= subviewId(); //m_id;

	map["maskfile"]	= m_alphaMaskFile;
	map["top"] 	= m_viewTop;
	map["left"] 	= m_viewLeft;
	map["right"] 	= m_viewRight;
	map["bottom"] 	= m_viewBottom;

	map["stop"] 	= m_sourceTop;
	map["sleft"] 	= m_sourceLeft;
	map["sright"] 	= m_sourceRight;
	map["sbottom"] 	= m_sourceBottom;

	map["fliph"]	= m_flipHorizontal;
	map["flipv"]	= m_flipVertical;

	map["b"]	= m_brightness;
	map["c"]	= m_contrast;
	map["h"]	= m_hue;
	map["s"]	= m_saturation;

	map["c1x"]	= m_cornerTranslations[0].x();
	map["c1y"]	= m_cornerTranslations[0].y();
	map["c2x"]	= m_cornerTranslations[1].x();
	map["c2y"]	= m_cornerTranslations[1].y();
	map["c3x"]	= m_cornerTranslations[2].x();
	map["c3y"]	= m_cornerTranslations[2].y();
	map["c4x"]	= m_cornerTranslations[3].x();
	map["c4y"]	= m_cornerTranslations[3].y();

	map["rot"]	= (int)m_cornerRotation;

	stream << map;

	return array;

}

void GLWidgetSubview::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
		return;

	m_title		= map["title"].toString();
	m_id		= map["id"].toInt();

	m_alphaMaskFile	= map["maskfile"].toString();

	m_viewTop 	= map["top"].toDouble();
	m_viewLeft	= map["left"].toDouble();
	m_viewRight	= map["right"].toDouble();
	m_viewBottom	= map["bottom"].toDouble();

	if(map["stop"].isValid())
	{
		m_sourceTop 	= map["stop"].toDouble();
		m_sourceLeft	= map["sleft"].toDouble();
		m_sourceRight	= map["sright"].toDouble();
		m_sourceBottom	= map["sbottom"].toDouble();
	}

	m_flipHorizontal = map["fliph"].toBool();
	m_flipVertical   = map["flipv"].toBool();

	m_brightness	= map["b"].toInt();
	m_contrast	= map["c"].toInt();
	m_hue		= map["h"].toInt();
	m_saturation	= map["s"].toInt();

	m_cornerTranslations	= QPolygonF()
		<< QPointF(map["c1x"].toDouble(),map["c1y"].toDouble())
		<< QPointF(map["c2x"].toDouble(),map["c2y"].toDouble())
		<< QPointF(map["c3x"].toDouble(),map["c3y"].toDouble())
		<< QPointF(map["c4x"].toDouble(),map["c4y"].toDouble());

	m_cornerRotation	= (GLRotateValue)map["rot"].toInt();

	setAlphaMaskFile(m_alphaMaskFile);
	m_colorsDirty = true;
	updateWarpMatrix();

	if(m_glw)
		m_glw->updateGL();

}

void GLWidgetSubview::initGL()
{
	initAlphaMask();
	updateWarpMatrix();
}

void GLWidgetSubview::setTitle(const QString& s) { m_title = s; }

void GLWidgetSubview::setTop(double d)
{
	m_viewTop = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setLeft(double d)
{
	m_viewLeft = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setBottom(double d)
{
	m_viewBottom = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setRight(double d)
{
	m_viewRight = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}


void GLWidgetSubview::setSourceTop(double d)
{
	m_sourceTop = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setSourceLeft(double d)
{
	m_sourceLeft = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setSourceBottom(double d)
{
	m_sourceBottom = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setSourceRight(double d)
{
	m_sourceRight = d;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}



GLWidget::GLWidget(QWidget *parent, QGLWidget *shareWidget)
	: QGLWidget(QGLFormat(QGL::SampleBuffers | QGL::AlphaChannel),parent, shareWidget)
	//: QGLWidget(parent, shareWidget)
	, m_glInited(false)
	, m_fbo(0)
	, m_aspectRatioMode(Qt::KeepAspectRatio)
	, m_program(0)
	, m_useShaders(false)
	, m_shadersLinked(false)
	, m_outputStream(0)
	, m_opacity(1.)
// 	, m_blackAnim(0)
	, m_isBlack(false)
	, m_crossfadeSpeed(300)
	, m_fboEnabled(false)
	//, m_fboEnabled(true)
	, m_readbackSizeAuto(false)
	//, m_readbackSizeAuto(true)
	, m_readbackFbo(0)
	, m_firstPbo(false)
	, m_backgroundColor(Qt::transparent) //Qt::black) 
{

	m_readbackSize = QSize(640,480);
	if(!m_fboEnabled)
		m_readbackSizeAuto= true;

	setCanvasSize(QSizeF(1000.,750.));
	// setViewport() will use canvas size by default to construct a rect
	//setViewport(QRectF(QPointF(0,0),canvasSize()));
	//qDebug() << "GLWidget::doubleBuffered: "<<doubleBuffer();
	(void)defaultSubview();

	connect(&m_fadeBlackTimer, SIGNAL(timeout()), this, SLOT(fadeBlackTick()));

	connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(callSuperUpdateGL()));
	m_updateTimer.setSingleShot(true);
	m_updateTimer.setInterval(0);





// 	GLWidgetSubview *def = defaultSubview();
// 	def->setRight(.5);
//
// 	GLWidgetSubview *test = new GLWidgetSubview();
// 	test->setLeft(.5);
// 	test->setHue(-50);
// 	test->setAlphaMaskFile("AlphaMaskTest2-right2.png");
// 	addSubview(test);

}

void GLWidget::callSuperUpdateGL()
{
	QGLWidget::updateGL();
	//qDebug() << QDateTime::currentDateTime()<<" QGLWidget::updateGL()";
}

void GLWidget::updateGL(bool now)
{
	if(m_updateTimer.isActive())
		m_updateTimer.stop();

	//now = false;
	if(now)
	{
		QGLWidget::updateGL();
		return;
	}

	m_updateTimer.start();
}

GLWidget::~GLWidget()
{
	while(!m_drawables.isEmpty())
	{
		GLDrawable *gld = m_drawables.takeFirst();
		removeDrawable(gld);
		if(!gld->scene())
		{
			delete gld;
			gld = 0;
		}
	}

	while(!m_subviews.isEmpty())
	{
		GLWidgetSubview *sub = m_subviews.takeFirst();
		removeSubview(sub);
		delete sub;
		sub = 0;
	}

	if(m_fbo)
	{
		delete m_fbo;
		m_fbo = 0;
	}
}

QSize GLWidget::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize GLWidget::sizeHint() const
{
	return QSize(400,300);
}

GLWidgetSubview *GLWidget::defaultSubview()
{
	if(m_subviews.isEmpty())
		addSubview(new GLWidgetSubview());

	return m_subviews[0];
}

GLWidgetSubview *GLWidget::subview(int subviewId)
{
	if(m_subviewLookup.contains(subviewId))
		return m_subviewLookup[subviewId];
	return 0;
}

void GLWidget::addSubview(GLWidgetSubview *s)
{
	if(!s)
		return;
	m_subviews << s;
	m_subviewLookup[s->subviewId()] = s;

	s->setGLWidget(this);
	if(m_glInited)
		s->initGL();

	if(m_subviews.size() > 1)
		setFboEnabled(true);

	updateGL();
}

void GLWidget::removeSubview(GLWidgetSubview *s)
{
	if(!s)
		return;

	s->setGLWidget(0);
	m_subviews.removeAll(s);
	m_subviewLookup.remove(s->subviewId());

	updateGL();
}

void GLWidget::setCornerTranslations(const QPolygonF& p)
{
	defaultSubview()->setCornerTranslations(p);
}

void GLWidgetSubview::setCornerTranslations(const QPolygonF& p)
{
	m_cornerTranslations = p;
	updateWarpMatrix();
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidget::setTopLeftTranslation(const QPointF& p)
{
	defaultSubview()->m_cornerTranslations[0] = p;
	defaultSubview()->updateWarpMatrix();
	updateGL();
}

void GLWidget::setTopRightTranslation(const QPointF& p)
{
	defaultSubview()->m_cornerTranslations[1] = p;
	defaultSubview()->updateWarpMatrix();
	updateGL();
}

void GLWidget::setBottomLeftTranslation(const QPointF& p)
{
	defaultSubview()->m_cornerTranslations[2] = p;
	defaultSubview()->updateWarpMatrix();
	updateGL();
}

void GLWidget::setBottomRightTranslation(const QPointF& p)
{
	defaultSubview()->m_cornerTranslations[3] = p;
	defaultSubview()->updateWarpMatrix();
	updateGL();
}

void GLWidget::initializeGL()
{
	makeCurrentIfNeeded();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_MULTISAMPLE)

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LINE_SMOOTH);

// 	glClearDepth(1.0f);						// Depth Buffer Setup
// 	glEnable(GL_DEPTH_TEST);					// Enables Depth Testing
// 	glDepthFunc(GL_LEQUAL);						// The Type Of Depth Testing To Do
// 	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);		// Really Nice Perspective Calculations

	//m_fbo = new QGLFramebufferObject(QSize(16,16));
	m_program = new QGLShaderProgram(context(), this);
	m_readbackProgram = new QGLShaderProgram(context(), this);

	#ifndef QT_OPENGL_ES
	glActiveTexture = (_glActiveTexture)context()->getProcAddress(QLatin1String("glActiveTexture"));
	#endif

	const GLubyte *str = glGetString(GL_EXTENSIONS);
	QString extensionList = QString((const char*)str);
	//m_useShaders = (strstr((const char *)str, "GL_ARB_fragment_shader") != NULL);
	m_useShaders = extensionList.indexOf("GL_ARB_fragment_shader") > -1;

	if(0)
	{
		qDebug() << "GLWidget::initGL: Forcing NO GLSL shaders";
		m_useShaders = false;
	}

	if(0)
	{
		qDebug() << "GLWidget::initGL: Forcing GLSL shaders";
		m_useShaders = true;
	}

	qDebug() << "GLWidget::initGL: GLSL Shaders Enabled: "<<m_useShaders;

	initShaders();

	m_glInited = true;

	foreach(GLWidgetSubview *view, m_subviews)
		view->initGL();


	//qDebug() << "GLWidget::initializeGL()";
	foreach(GLDrawable *drawable, m_drawables)
		drawable->initGL();

	#ifdef Q_OS_WIN32
		#if 0
		// check PBO is supported by your video card
		if(extensionList.indexOf("GL_ARB_pixel_buffer_object") > -1)
		{
			// get pointers to GL functions
	// 		glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)context()->getProcAddress(QLatin1String("glGenBuffersARB"));
	// 		glBindBufferARB = (PFNGLBINDBUFFERARBPROC)context()->getProcAddress(QLatin1String("glBindBufferARB"));
	// 		glBufferDataARB = (PFNGLBUFFERDATAARBPROC)context()->getProcAddress(QLatin1String("glBufferDataARB"));
	// 		glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)context()->getProcAddress(QLatin1String("glBufferSubDataARB"));
	// 		glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)context()->getProcAddress(QLatin1String("glDeleteBuffersARB"));
	// 		glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)context()->getProcAddress(QLatin1String("glGetBufferParameterivARB"));
	// 		glMapBufferARB = (PFNGLMAPBUFFERARBPROC)context()->getProcAddress(QLatin1String("glMapBufferARB"));
	// 		glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)context()->getProcAddress(QLatin1String("glUnmapBufferARB"));

			// get pointers to GL functions
			glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
			glBindBufferARB = (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
			glBufferDataARB = (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
			glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
			glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
			glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
			glMapBufferARB = (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
			glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");


			// check once again PBO extension
			if(glGenBuffersARB && glBindBufferARB && glBufferDataARB && glBufferSubDataARB &&
			glMapBufferARB && glUnmapBufferARB && glDeleteBuffersARB && glGetBufferParameterivARB)
			{
				m_pboEnabled = true;
				qDebug() << "Video card supports GL_ARB_pixel_buffer_object.";
			}
			else
			{
				m_pboEnabled = false;
				qDebug() << "Video card does NOT support GL_ARB_pixel_buffer_object.";
			}
		}
		#else
			m_pboEnabled = false;
		#endif

 	#else // for linux, do not need to get function pointers, it is up-to-date
	if(extensionList.indexOf("GL_ARB_pixel_buffer_object") > -1)
	{
		m_pboEnabled = true;
		qDebug() << "Video card supports GL_ARB_pixel_buffer_object.";
	}
	else
	{
		m_pboEnabled = false;
		qDebug() << "Video card does NOT support GL_ARB_pixel_buffer_object.";
	}
 	#endif

	//resizeGL(width(),height());
	//setViewport(viewport());
	//updateWarpMatrix();

}

static const char *glwidget_glsl_simpleVertexShaderProgram =
        "attribute highp vec4 vertexCoordArray;\n"
        "attribute highp vec2 textureCoordArray;\n"
        "uniform highp mat4 positionMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "   gl_Position = positionMatrix * vertexCoordArray;\n"
        //"   gl_Position = vertexCoordArray\n"// * gl_ModelViewProjectionMatrix;\n"
        "   textureCoord = textureCoordArray;\n"
        "}\n";


static const char *glwidget_glsl_rgbNoAlphaShaderProgram =
        "uniform sampler2D texRgb;\n"
        //"uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    mediump vec2 texPoint = vec2(textureCoord.s, textureCoord.t);\n"
        "    highp vec4 color = vec4(texture2D(texRgb, texPoint).rgb, 1.0);\n"
        //"    color = colorMatrix * color;\n"
        "    gl_FragColor = color;\n"
        //vec4(color.rgb, texture2D(texRgb, texPoint).a * alpha * texture2D(alphaMask, textureCoord.st).a);\n"
        "}\n";

static const char *glwidget_glsl_rgbShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform sampler2D alphaMask;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "uniform mediump float alpha;\n"
        "uniform mediump float texOffsetX;\n"
        "uniform mediump float texOffsetY;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    mediump vec2 texPoint = vec2(textureCoord.s + texOffsetX, textureCoord.t + texOffsetY);\n"
        "    highp vec4 color = vec4(texture2D(texRgb, texPoint).rgb, 1.0);\n"
        "    color = colorMatrix * color;\n"
        "    gl_FragColor = vec4(color.rgb, texture2D(texRgb, texPoint).a * alpha * texture2D(alphaMask, textureCoord.st).a);\n"
        "}\n";


void GLWidget::initShaders()
{
	#ifdef OPENCV_ENABLED
	Q_UNUSED(qt_glsl_vertexShaderProgram);
	#else
	Q_UNUSED(qt_glsl_warpingVertexShaderProgram);
	#endif
	
	Q_UNUSED(qt_glsl_xrgbShaderProgram);
	Q_UNUSED(qt_glsl_argbShaderProgram);
	Q_UNUSED(qt_glsl_rgbShaderProgram);
// 	Q_UNUSED(qt_glsl_xrgbLevelsShaderProgram);
// 	Q_UNUSED(qt_glsl_argbLevelsShaderProgram);
// 	Q_UNUSED(qt_glsl_rgbLevelsShaderProgram);
	Q_UNUSED(qt_glsl_yuvPlanarShaderProgram);
	Q_UNUSED(qt_glsl_xyuvShaderProgram);
	Q_UNUSED(qt_glsl_ayuvShaderProgram);
	Q_UNUSED(qt_glsl_uyvyShaderProgram);
	Q_UNUSED(qt_glsl_yuyvShaderProgram);
	
	const char *fragmentProgram = glwidget_glsl_rgbShaderProgram;
	
	if(!m_program->shaders().isEmpty())
		m_program->removeAllShaders();

	if(!QGLShaderProgram::hasOpenGLShaderPrograms())
	{
		qDebug() << "GLWidget::initShaders: GLSL Shaders Not Supported by this driver, this program will NOT function as expected and will likely crash.";
		return;
	}

	if (!fragmentProgram)
	{
		qDebug() << "GLWidget::initShaders: No shader program found - format not supported.";
		return;
	}
	else
	if (!m_program->addShaderFromSourceCode(QGLShader::Vertex,
		#ifdef OPENCV_ENABLED
		qt_glsl_warpingVertexShaderProgram
		#else
		qt_glsl_vertexShaderProgram
		#endif
		))
	{
		qWarning("GLWidget::initShaders: Vertex shader compile error %s", qPrintable(m_program->log()));
		return;

	}
	else
	if (!m_program->addShaderFromSourceCode(QGLShader::Fragment, fragmentProgram))
	{
		qDebug() << "GLWidget::initShaders: Fragment program used: \n"<<fragmentProgram; //glwidget_glsl_rgbShaderProgram;
		qWarning("GLWidget::initShaders: Shader compile error %s", qPrintable(m_program->log()));
		m_program->removeAllShaders();
		return;
	}
	else
	if(!m_program->link())
	{
		qWarning("GLWidget::initShaders: Shader link error %s", qPrintable(m_program->log()));
		m_program->removeAllShaders();
		return;
	}

	m_shadersLinked = true;
	
	
	// Setup readback shaders
	const char *readbackFragment = glwidget_glsl_rgbNoAlphaShaderProgram;
	
	if(!m_readbackProgram->shaders().isEmpty())
		m_readbackProgram->removeAllShaders();

	if (!m_readbackProgram->addShaderFromSourceCode(QGLShader::Vertex, 
		glwidget_glsl_simpleVertexShaderProgram))
		//qt_glsl_vertexShaderProgram))
	{
		qWarning("GLWidget::initShaders: Readback Shader: Vertex shader compile error %s", qPrintable(m_readbackProgram->log()));
		return;
	}
	else
	if (!m_readbackProgram->addShaderFromSourceCode(QGLShader::Fragment, readbackFragment))
	{
		qWarning("GLWidget::initShaders: Readback Shader: Shader compile error %s", qPrintable(m_readbackProgram->log()));
		m_readbackProgram->removeAllShaders();
		return;
	}
	else
	if(!m_readbackProgram->link())
	{
		qWarning("GLWidget::initShaders: Readback Shader: Shader link error %s", qPrintable(m_readbackProgram->log()));
		m_readbackProgram->removeAllShaders();
		return;
	}

	m_readbackShadersLinked = true;
}

void GLWidgetSubview::initAlphaMask()
{
	glGenTextures(1, &m_alphaTextureId);

	// Alpha mask may have been set prior to initAlphaMask() due to the fact init...() is called from initalizeGL()
	if(m_alphaMask.isNull())
	{
		m_alphaMask = QImage(1,1,QImage::Format_RGB32);
		m_alphaMask.fill(Qt::black);
 		//qDebug() << "GLVideoDrawable::initGL: BLACK m_alphaMask.size:"<<m_alphaMask.size();
 		setAlphaMask(m_alphaMask);
	}
	else
	{
 		//qDebug() << "GLVideoDrawable::initGL: Alpha mask already set, m_alphaMask.size:"<<m_alphaMask.size();
		setAlphaMask(m_alphaMask_preScaled);
	}
}

void GLWidget::showEvent(QShowEvent *)
{
	//qDebug() << "GLWidget::showEvent(QShowEvent *)";
	QTimer::singleShot(1050,this,SLOT(postInitGL()));
	QTimer::singleShot(2050,this,SLOT(postInitGL()));
	QTimer::singleShot(3050,this,SLOT(postInitGL()));
}


void GLWidget::postInitGL()
{
	resizeGL(width(),height());
}


void GLWidget::makeRenderContextCurrent()
{
	if(m_fbo)
	{
		if(!m_fbo->isBound())
		{
			//QTime t;
			//t.start();
			m_fbo->bind();
			//qDebug() << "GLWidget::makeRenderContextCurrent: Called FBO bind(), time:"<<t.elapsed();
		}
	}
	else
	{
		makeCurrentIfNeeded();
	}
}

void GLWidget::makeCurrentIfNeeded()
{
	if(QGLContext::currentContext() != context())
	{
		//QTime t;
		//t.start();
		makeCurrent();
		//qDebug() << "GLWidget::makeCurrentIfNeeded: Called makeCurrent(), time:"<<t.elapsed();
	}
}

void GLWidget::setCornerRotation(GLRotateValue rv)
{
	defaultSubview()->setCornerRotation(rv);
	updateGL();
}

void GLWidgetSubview::setCornerRotation(GLRotateValue rv)
{
	m_cornerRotation = rv;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidget::setAlphaMask(const QImage &mask)
{
	defaultSubview()->setAlphaMask(mask);
}

void GLWidgetSubview::setAlphaMaskFile(const QString &file)
{
	setAlphaMask(QImage(file));
}

void GLWidgetSubview::setAlphaMask(const QImage &mask)
{
	m_alphaMask_preScaled = mask;
	m_alphaMask = mask;

	if(mask.isNull())
	{
		//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Got null mask, size:"<<mask.size();
		//return;
		m_alphaMask = QImage(1,1,QImage::Format_RGB32);
		m_alphaMask.fill(Qt::black);
 		//qDebug() << "GLVideoDrawable::initGL: BLACK m_alphaMask.size:"<<m_alphaMask.size();
 		setAlphaMask(m_alphaMask);
 		return;
	}

	if(m_glw && m_glw->glInited())
	{
		QSize targetSize = targetRect().size().toSize(); //QSize(m_glw->width(),m_glw->height()); //m_fbo->size();
		if(targetSize == QSize(0,0))
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<", Not scaling or setting mask, video size is 0x0";
			return;
		}

		m_glw->makeCurrentIfNeeded();


// 		if(m_alphaMask.size() != targetSize)
// 		{
// 			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Mask size and source size different, scaling";
// 			m_alphaMask = m_alphaMask.scaled(targetSize);
// 		}

// 		if(m_alphaMask.format() != QImage::Format_ARGB32)
// 		{
// 			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Mask format not ARGB32, reformatting";
// 			m_alphaMask = m_alphaMask.convertToFormat(QImage::Format_ARGB32);
// 		}

		if(m_alphaMask.cacheKey() == m_uploadedCacheKey)
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Current mask already uploaded to GPU, not re-uploading.";
			return;
		}

		m_alphaMask = m_alphaMask.mirrored(m_flipHorizontal, m_flipVertical ? false:true);

		m_uploadedCacheKey = m_alphaMask.cacheKey();

		 //qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Valid mask, size:"<<m_alphaMask.size()<<", null?"<<m_alphaMask.isNull();

		glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			m_alphaMask.width(),
			m_alphaMask.height(),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			m_alphaMask.scanLine(0)
			);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	emit changed(this);
// 	qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  AT END :"<<m_alphaMask.size()<<", null?"<<m_alphaMask.isNull();

}

QRectF GLWidgetSubview::targetRect()
{
	if(!m_glw)
		return QRectF();

	double targetW = m_glw->width(),
	       targetH = m_glw->height();
	double targetX = targetW * left();
	double targetY = targetH * top();
	double targetW2 = (targetW * right()) - targetX;
	double targetH2 = (targetH * bottom()) - targetY;
	QRectF target = QRectF(targetX,targetY, targetW2, targetH2);
	return target;
}

void GLWidgetSubview::updateWarpMatrix()
{
	#ifdef OPENCV_ENABLED
	if(!m_glw)
		return;

	// Handle flipping the corner translation points so that the top left is flipped
	// without the user having think about flipping the point inputs.
	int     cbl = 2,
		cbr = 3,
		ctl = 0,
		ctr = 1;

	if(m_flipHorizontal && m_flipVertical)
	{
		cbl = 1;
		cbr = 0;
		ctl = 3;
		ctr = 2;
	}
	else
	if(m_flipVertical)
	{
		cbl = 0;
		cbr = 1;
		ctl = 2;
		ctr = 3;
	}
	else
	if(m_flipHorizontal)
	{
		cbl = 1;
		cbr = 0;
		ctl = 3;
		ctr = 2;
	}

	QRectF target = targetRect();

	//qDebug() << "Original painting target: "<<target;

	//we need our points as opencv points
	//be nice to do this without opencv?
	CvPoint2D32f cvsrc[4];
	CvPoint2D32f cvdst[4];

	// Warp source coordinates
	cvsrc[0].x = target.left();
	cvsrc[0].y = target.top();
	cvsrc[1].x = target.right();
	cvsrc[1].y = target.top();
	cvsrc[2].x = target.right();
	cvsrc[2].y = target.bottom();
	cvsrc[3].x = target.left();
	cvsrc[3].y = target.bottom();

	// Warp destination coordinates
	cvdst[0].x = target.left()	+ m_cornerTranslations[ctl].x();
	cvdst[0].y = target.top()	+ m_cornerTranslations[ctl].y();
	cvdst[1].x = target.right()	- m_cornerTranslations[ctr].x();
	cvdst[1].y = target.top()	+ m_cornerTranslations[ctr].y();
	cvdst[2].x = target.right()	- m_cornerTranslations[cbr].x();
	cvdst[2].y = target.bottom()	- m_cornerTranslations[cbr].y();
	cvdst[3].x = target.left()	+ m_cornerTranslations[cbl].x();
	cvdst[3].y = target.bottom()	- m_cornerTranslations[cbl].y();


	//we create a matrix that will store the results
	//from openCV - this is a 3x3 2D matrix that is
	//row ordered
	CvMat * translate = cvCreateMat(3,3,CV_32FC1);

	//this is the slightly easier - but supposidly less
	//accurate warping method
	//cvWarpPerspectiveQMatrix(cvsrc, cvdst, translate);


	//for the more accurate method we need to create
	//a couple of matrixes that just act as containers
	//to store our points  - the nice thing with this
	//method is you can give it more than four points!

	CvMat* src_mat = cvCreateMat( 4, 2, CV_32FC1 );
	CvMat* dst_mat = cvCreateMat( 4, 2, CV_32FC1 );

	//copy our points into the matrixes
	cvSetData( src_mat, cvsrc, sizeof(CvPoint2D32f));
	cvSetData( dst_mat, cvdst, sizeof(CvPoint2D32f));

	//figure out the warping!
	//warning - older versions of openCV had a bug
	//in this function.
	cvFindHomography(src_mat, dst_mat, translate);

	//get the matrix as a list of floats
	float *matrix = translate->data.fl;

	//we need to copy these values
	//from the 3x3 2D openCV matrix which is row ordered
	//
	// ie:   [0][1][2] x
	//       [3][4][5] y
	//       [6][7][8] w

	//to openGL's 4x4 3D column ordered matrix
	//        x  y  z  w
	// ie:   [0][3][ ][6]   [1-4]
	//       [1][4][ ][7]   [5-8]
	//	 [ ][ ][ ][ ]   [9-12]
	//       [2][5][ ][9]   [13-16]
	//


	m_warpMatrix[0][0] = matrix[0];
	m_warpMatrix[0][1] = matrix[3];
	m_warpMatrix[0][2] = 0.;
	m_warpMatrix[0][3] = matrix[6];

	m_warpMatrix[1][0] = matrix[1];
	m_warpMatrix[1][1] = matrix[4];
	m_warpMatrix[1][2] = 0.;
	m_warpMatrix[1][3] = matrix[7];

	m_warpMatrix[2][0] = 0.;
	m_warpMatrix[2][1] = 0.;
	m_warpMatrix[2][2] = 0.;
	m_warpMatrix[2][3] = 0.;

	m_warpMatrix[3][0] = matrix[2];
	m_warpMatrix[3][1] = matrix[5];
	m_warpMatrix[3][2] = 0.;
	m_warpMatrix[3][3] = matrix[8];

	cvReleaseMat(&translate);
	cvReleaseMat(&src_mat);
	cvReleaseMat(&dst_mat);



// 	qDebug() << "Warp Matrix: "
// 		<<matrix[0]
// 		<<matrix[1]
// 		<<matrix[2]
// 		<<matrix[3]
// 		<<matrix[4]
// 		<<matrix[5]
// 		<<matrix[6]
// 		<<matrix[7]
// 		<<matrix[8];
	#endif
}


/*
   Read back the contents of the currently bound framebuffer, used in
   QGLWidget::grabFrameBuffer(), QGLPixelbuffer::toImage() and
   QGLFramebufferObject::toImage()
*/

static void convertFromGLImage(QImage &img, int w, int h, bool alpha_format, bool include_alpha)
{
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        // OpenGL gives RGBA; Qt wants ARGB
        uint *p = (uint*)img.bits();
        uint *end = p + w*h;
        if (alpha_format && include_alpha) {
            while (p < end) {
                uint a = *p << 24;
                *p = (*p >> 8) | a;
                p++;
            }
        } else {
            // This is an old legacy fix for PowerPC based Macs, which
            // we shouldn't remove
            while (p < end) {
                *p = 0xff000000 | (*p>>8);
                ++p;
            }
        }
    } else {
        // OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
        for (int y = 0; y < h; y++) {
            uint *q = (uint*)img.scanLine(y);
            for (int x=0; x < w; ++x) {
                const uint pixel = *q;
                *q = ((pixel << 16) & 0xff0000) | ((pixel >> 16) & 0xff) | (pixel & 0xff00ff00);
                q++;
            }
        }

    }
    img = img.mirrored();
}

QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha)
{
    QImage img(size, alpha_format ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    int w = size.width();
    int h = size.height();
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    convertFromGLImage(img, w, h, alpha_format, include_alpha);
    return img;
}

QImage qt_gl_read_texture(const QSize &size, bool alpha_format, bool include_alpha)
{
    QImage img(size, alpha_format ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
    int w = size.width();
    int h = size.height();
#if !defined(QT_OPENGL_ES_2) && !defined(QT_OPENGL_ES_1) && !defined(QT_OPENGL_ES_1_CL)
    //### glGetTexImage not in GL ES 2.0, need to do something else here!
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
#endif
    convertFromGLImage(img, w, h, alpha_format, include_alpha);
    return img;
}
void GLWidget::paintGL()
{
	//qDebug() << "GLWidget::paintGL(): Starting paint routines...";
// 	QTime time;
// 	time.start();
// 	if(!isVisible())
// 		return;
		
	//qDebug() << "GLWidget::paintGL(): Starting paint routines...";

	// Render all drawables into the FBO
	//if(m_fbo)
	//	m_fbo->bind();
	makeRenderContextCurrent();

	qglClearColor(m_backgroundColor); //Qt::black);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity(); // Reset The View

	QRectF viewport = m_viewport;
	if(!viewport.isValid())
	{
		QSizeF canvas = m_canvasSize;
		if(canvas.isNull() || !canvas.isValid())
			canvas = QSizeF(1000.,750.);
		viewport = QRectF(QPointF(0.,0.),canvas);
	}

	//int counter = 0;
	foreach(GLDrawable *drawable, m_drawables)
	{
		//qDebug() << "GLWidget::paintGL(): ["<<counter++<<"] drawable->rect: "<<drawable->rect();

 		//qDebug() << "GLWidget::paintGL(): drawable:"<<((QObject*)drawable)<<", vp:"<<viewport<<", rect:"<<drawable->rect()<<", isvis:"<<drawable->isVisible()<<", opac:"<<drawable->opacity()<<", intersects:"<<drawable->rect().intersects(viewport)<<", z:"<<drawable->zIndex();
		// Don't draw if not visible or if opacity == 0
		if(drawable->isVisible() &&
		   drawable->opacity() > 0 &&
		   drawable->rect().intersects(viewport))
		   {
			//qDebug() << "GLWidget::paintGL(): drawable:"<<((QObject*)drawable);
			drawable->paintGL();
		   }
// 		qDebug() << "GLWidget::paintGL(): drawable:"<<((void*)drawable)<<", draw done";
	}

	//qDebug() << "GLWidget::paintGL(): Drawable painting complete, drawing FBO";

	//glFlush();

	if(m_outputStream)
	{
 		if(m_fbo)
 		{
			
			QTime t;
			t.start();
			//qDebug() << "GLWidget::paintGL(): ----------------------------------------------";
			
			if(m_fbo)
			{
				if(m_fbo->isBound())
					m_fbo->release();
				//qDebug() << "GLWidget::paintGL(): FBO enabled, releasing FBO for read";
			}
			else
			{
// 				glFlush();
// 				glFinish();
				//glReadBuffer(GL_FRONT);
 				glBindTexture(GL_TEXTURE_2D, m_readbackTextureId);
 				glCopyTexSubImage2D(GL_TEXTURE_2D,
 					0, // level 
 					0, 0, // x,y offset
 					0, 0, // x,y
 					width(), height());
// 			
				//glBindTexture(GL_TEXTURE_2D, 0);
				qDebug() << "GLWidget::paintGL(): Read from screen buffer to m_readbackTextureId took"<<t.restart()<<"ms";
				
				qDebug() << "GLWidget::paintGL(): ----------------------------------------------";
				
				QSize sz = size();
				//QImage img(sz, QImage::Format_RGB32);
				//### glGetTexImage not in GL ES 2.0, need to do something else here!
				//glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());
				
//				QImage img = qt_gl_read_texture(sz, true, true);
				
// 				bool alpha_format = true;
// 				bool include_alpha = true;
// 				
// 				QImage img(sz, alpha_format ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
// 				int w = sz.width();
// 				int h = sz.height();
// 				#if !defined(QT_OPENGL_ES_2) && !defined(QT_OPENGL_ES_1) && !defined(QT_OPENGL_ES_1_CL)
// 				//### glGetTexImage not in GL ES 2.0, need to do something else here!
// 				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
// 				#endif
// 				convertFromGLImage(img, w, h, alpha_format, include_alpha);
// 				//return img;
	
				
// 				static int outCount = 0;
// 				outCount ++;
// 				QString file = QString("debug/gettex-%1.png").arg(outCount % 2 == 0 ? 0 : 1);
// 				img.save(file);
// 	                        qDebug() << "GLWidget::paintGL(): Saving debug copy of m_readbackTextureId to "<<file<<" took"<<t.restart()<<"ms";
				
			}
				
			//qDebug() << "GLWidget::paintGL(): Rendering screen image to readback FBO";
			m_readbackFbo->bind();
			
			// Dont need to clear since we KNOW we are filling the entire FBO with the texture
			qglClearColor(m_backgroundColor);//Qt::black);
			
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity(); // Reset The View
			glEnable(GL_TEXTURE_2D);
// 			
 			//glBindTexture(GL_TEXTURE_2D, m_fbo ? m_fbo->texture() : m_readbackTextureId);
 			
//  			QImage texOrig, texGL;
// 			if ( !texOrig.load( "me2.jpg" ) )
// 			{
// 				texOrig = QImage( 16, 16, QImage::Format_RGB32 );
// 				texOrig.fill( Qt::green );
// 			}
// 			
// 		// 	if(texOrig.format() != QImage::Format_RGB32)
// 		// 		texOrig = texOrig.convertToFormat(QImage::Format_RGB32);
// 			
// 			texGL = QGLWidget::convertToGLFormat( texOrig );
// 			
//  			
//  			glBindTexture(GL_TEXTURE_2D, m_readbackTextureId);
//  			
//  			glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
// 			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
// 			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
// 
// 			//glTranslatef(0.0f,0.0f,-3.42f);
// 
			//qDebug() << "GLWidget::paintGL(): Readback FBO setup took"<<t.restart()<<"ms";
			
 			//QRectF rect(0,0,m_readbackFbo->width(),m_readbackFbo->height());
 			//qDebug() << "GLWidget::paintGL(): Readback rect:"<<rect;

//  			glBegin(GL_QUADS);
// // 			
// 
// 			qreal
// 				vx1 = rect.left(),
// 				vx2 = rect.right(),
// 				vy1 = rect.bottom(),
// 				vy2 = rect.top();
// 
// 			glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1,vy1,  0.0f); // bottom left // 3
// 			glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2,vy1,  0.0f); // bottom right // 2
// 			glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2,vy2,  0.0f); // top right  // 1
// 			glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1,vy2,  0.0f); // top left // 0
// 
// 			glEnd();


			{
			
				const int devW = m_readbackFbo->width(); //QGLContext::currentContext()->device()->width();
				const int devH = m_readbackFbo->height(); //QGLContext::currentContext()->device()->height();
	
				QTransform transform =  QTransform();
	
				const GLfloat wfactor =  2.0 / devW;
				const GLfloat hfactor = -2.0 / devH;
	
				const GLfloat positionMatrix[4][4] =
				{
					{
						/*(0,0)*/ wfactor * transform.m11() - transform.m13(),
						/*(0,1)*/ hfactor * transform.m12() + transform.m13(),
						/*(0,2)*/ 0.0,
						/*(0,3)*/ transform.m13()
					}, {
						/*(1,0)*/ wfactor * transform.m21() - transform.m23(),
						/*(1,1)*/ hfactor * transform.m22() + transform.m23(),
						/*(1,2)*/ 0.0,
						/*(1,3)*/ transform.m23()
					}, {
						/*(2,0)*/ 0.0,
						/*(2,1)*/ 0.0,
						/*(2,2)*/ -1.0,
						/*(2,3)*/ 0.0
					}, {
						/*(3,0)*/ wfactor * transform.dx() - transform.m33(),
						/*(3,1)*/ hfactor * transform.dy() + transform.m33(),
						/*(3,2)*/ 0.0,
						/*(3,3)*/ transform.m33()
					}
				};
				
				//
				QRectF source = QRectF(m_readbackFbo->height() - height(),width() + (width() - m_readbackFbo->width()),width(),height());

				QRectF target = QRectF(0,0,m_readbackFbo->width(),m_readbackFbo->height());

				const GLfloat vertexCoordArray[] =
				{
					target.left()      , target.bottom() + 1 ,
					target.right() + 1 , target.bottom() + 1 ,
					target.left()      , target.top()        ,
					target.right() + 1 , target.top()
				};
				
				/// Unused currently
// 				bool flipHorizontal = false;
// 				bool flipVertical = false;
				
				/// Unused currently
// 				double sourceWidth  = (double)width();
// 				double sourceHeight = (double)height();
				
				const GLfloat txLeft   = 0; //flipHorizontal ? source.right()  / sourceWidth : source.left()  / sourceWidth;
				const GLfloat txRight  = 1.25; //flipHorizontal ? source.left()   / sourceWidth : source.right() / sourceWidth;
				
				const GLfloat txTop    = -.25;
				const GLfloat txBottom = 1;

// 				const GLfloat txTop    = flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
// 					? source.top()    / sourceHeight
// 					: source.bottom() / sourceHeight;
// 				const GLfloat txBottom = flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
// 					? source.bottom() / sourceHeight
// 					: source.top()    / sourceHeight;

				GLfloat textureCoordArray[] =
				{
					txLeft , txBottom,
					txRight, txBottom,
					txLeft , txTop,
					txRight, txTop
				};
				
				//qDebug() << "GLWidget::paintGL(): tx coords:"<<txLeft<<","<<txTop<<","<<txRight<<","<<txBottom;
				//qDebug() << "GLWidget::paintGL(): target:"<<target;

				m_readbackProgram->bind();

				m_readbackProgram->enableAttributeArray("vertexCoordArray");
				m_readbackProgram->enableAttributeArray("textureCoordArray");

				m_readbackProgram->setAttributeArray("vertexCoordArray",  vertexCoordArray,  2);
				m_readbackProgram->setAttributeArray("textureCoordArray", textureCoordArray, 2);

				m_readbackProgram->setUniformValue("positionMatrix",      positionMatrix);
			
				glActiveTexture(GL_TEXTURE0);
				//glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
				//glBindTexture(GL_TEXTURE_2D, m_readbackTextureId);
				glBindTexture(GL_TEXTURE_2D, m_fbo ? m_fbo->texture() : m_readbackTextureId);

		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				m_readbackProgram->setUniformValue("texRgb", 0);
				
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				//glDrawArrays(GL_QUAD_STRIP, 0, 4);

				m_readbackProgram->release();
			
			
			}
			
			
			
			//qDebug() << "GLWidget::paintGL(): Readback FBO render took"<<t.restart()<<"ms";
			
 			QImage tmp = m_readbackFbo->toImage();
 			m_outputStream->setImage(tmp);
			
			
// 			static int outCount = 0;
// 			outCount ++;
// 			QString file = QString("debug/readback-%1.png").arg(outCount % 2 == 0 ? 0 : 1);
// 			tmp.save(file);
// 			
// 			qDebug() << "GLWidget::paintGL(): Readback FBO debugging write to "<<file<<" took "<<t.restart()<<"ms";
// 			m_readbackFbo->release();
// 			m_readbackFbo->bind();
 		}
// 		else
// 		{
// 			if(m_fbo && 
// 			  !m_fbo->isBound())
// 				m_fbo->bind();
// 		}
			
		else
			
		//qDebug() << "GLWidget::paintGL(): Downloading from GPU to m_outputStream";
// 		QTime t;
// 		t.start();
		#ifndef Q_OS_WIN32
		if(m_pboEnabled)
		{
			//m_readbackFbo->bind();
			
			m_firstPbo = ! m_firstPbo;
			int processIdx = m_firstPbo ? 0 : 1;
			int readIdx = m_firstPbo ? 1 :0;

			//qDebug() << "GLWidget::paintGL(): Read texture size:"<<size()<<", output size:"<<m_readbackSize<<", readIdx:"<<readIdx<<", processIdx:"<<processIdx;

			//glFinish();

			//glReadBuffer(GL_FRONT);
			// read pixels from framebuffer to PBO
			// glReadPixels() should return immediately.
			
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pboIds[readIdx]);

//  			QTime t;
//  			t.start();

			glReadPixels(0, 0, m_readbackSize.width(), m_readbackSize.height(), GL_BGRA, GL_UNSIGNED_BYTE, 0);

//  			int elapsed = t.elapsed();
//  			t.restart();

			// map the PBO to process its data by CPU
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pboIds[processIdx]);
			GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB,
								GL_READ_ONLY_ARB);
			if(ptr)
			{
//  				QImage img(m_readbackSize, QImage::Format_ARGB32);
//  				memcpy(img.bits(), ptr, img.byteCount());
// 				m_outputStream->setImage(img);
				m_outputStream->copyPtr(ptr, size());

// 				int elapsed2 = t.elapsed();
// 				//m_outputStream->setImage(img);
// 				QString file = QString("debug/pboread-%1.png").arg(readIdx);
// 				img.save(file);
// 				qDebug() << "Writing to file:"<<QString("debug/pboread-%1.png").arg(readIdx)<<", elapsed:"<<elapsed;
// 				qDebug() << "Download from buffer "<<readIdx<<" completed, "<<elapsed<<"ms. Image buffer "<<processIdx<<" processed, "<<elapsed2<<" ms";

				glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			else
			{
				qDebug() << "GLWidget::paintGL(): No ptr received from glMapBufferARB()";
			}

			// back to conventional pixel operation
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
		}
// 		else
 		#else
		{

// 			QTime t;
// 			t.start();

			QImage img(size(), QImage::Format_ARGB32);
			glReadPixels(0, 0, width(), height(), GL_BGRA, GL_UNSIGNED_BYTE, img.bits());
			m_outputStream->setImage(img);
			//qDebug() << "glReadPixels elapsed:"<<t.elapsed()<<"ms";
		}
		#endif
		
		if(m_readbackFbo && 
		   m_readbackFbo->isBound())
			m_readbackFbo->release();


	}
	
	if(m_fbo)
	{
		m_fbo->release();
		//qDebug() << "GLWidget::paintGL(): Downloading from GPU to m_outputStream - via FBO";

		// Now render the FBO to the screen, applying a variety of transforms/effects:
		// 1. Corner distortion ("keystoning") - can move any of the four corners individually
		// 2. Alpha masking - using a PNG image as a mask, using only the alpha channel to blend/black out areas of the final image
		// 3. Brightness/Contrast/Hue/Saturation adjustments - Adjust the B/C/H/S over the entire output image, not just individual drawables
		// The alpha masking and BCHS adjustments require pixel shaders - therefore, if the system does not support them (<OpenGL 2), then
		// only the first item (corner distortion) will work.
		qglClearColor(m_backgroundColor);//Qt::black);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity(); // Reset The View

		glEnable(GL_TEXTURE_2D);

		if(m_useShaders &&
		   m_shadersLinked)
		{

			const int devW = QGLContext::currentContext()->device()->width();
			const int devH = QGLContext::currentContext()->device()->height();

			QTransform transform =  QTransform();

			const GLfloat wfactor =  2.0 / devW;
			const GLfloat hfactor = -2.0 / devH;

			const GLfloat positionMatrix[4][4] =
			{
				{
					/*(0,0)*/ wfactor * transform.m11() - transform.m13(),
					/*(0,1)*/ hfactor * transform.m12() + transform.m13(),
					/*(0,2)*/ 0.0,
					/*(0,3)*/ transform.m13()
				}, {
					/*(1,0)*/ wfactor * transform.m21() - transform.m23(),
					/*(1,1)*/ hfactor * transform.m22() + transform.m23(),
					/*(1,2)*/ 0.0,
					/*(1,3)*/ transform.m23()
				}, {
					/*(2,0)*/ 0.0,
					/*(2,1)*/ 0.0,
					/*(2,2)*/ -1.0,
					/*(2,3)*/ 0.0
				}, {
					/*(3,0)*/ wfactor * transform.dx() - transform.m33(),
					/*(3,1)*/ hfactor * transform.dy() + transform.m33(),
					/*(3,2)*/ 0.0,
					/*(3,3)*/ transform.m33()
				}
			};


			foreach(GLWidgetSubview *view, m_subviews)
			{

				if (view->m_colorsDirty)
				{
					//qDebug() << "Updating color matrix";
					view->updateColors();
					view->m_colorsDirty = false;
				}




				double sw = m_fbo->size().width(),
				sh = m_fbo->size().height();

				double sl = view->sourceLeft()   < 0 ? view->left()   : view->sourceLeft();
				double st = view->sourceTop()    < 0 ? view->top()    : view->sourceTop();
				double sr = view->sourceRight()  < 0 ? view->right()  : view->sourceRight();
				double sb = view->sourceBottom() < 0 ? view->bottom() : view->sourceBottom();

				double sx = sw * sl;
				double sy = sw * st;
				double sw2 = (sw * sr) - sx;
				double sh2 = (sh * sb) - sy;
				QRectF source = QRectF(sx,sy,sw2,sh2);

				QRectF target = view->targetRect();

				#ifdef OPENCV_ENABLED

				const GLfloat vertexCoordArray[] =
				{
					target.left()      , target.bottom() + 1 ,
					target.right() + 1 , target.bottom() + 1 ,
					target.left()      , target.top()        ,
					target.right() + 1 , target.top()
				};

				#else

				// Handle flipping the corner translation points so that the top left is flipped
				// without the user having think about flipping the point inputs.
				int cbl = 2,
				cbr = 3,
				ctl = 0,
				ctr = 1;

				if(view->m_flipHorizontal && view->m_flipVertical)
				{
					cbl = 1;
					cbr = 0;
					ctl = 3;
					ctr = 2;
				}
				else
				if(view->m_flipVertical)
				{
					cbl = 0;
					cbr = 1;
					ctl = 2;
					ctr = 3;
				}
				else
				if(view->m_flipHorizontal)
				{
					cbl = 1;
					cbr = 0;
					ctl = 3;
					ctr = 2;
				}

				//qDebug() << "cbl:"<<cbl<<",cbr:"<<cbr<<",ctl:"<<ctl<<",ctr:"<<ctr;

				const GLfloat vertexCoordArray[] =
				{
					target.left()      + view->m_cornerTranslations[cbl].x(), target.bottom() + 1 - view->m_cornerTranslations[cbl].y(),
					target.right() + 1 - view->m_cornerTranslations[cbr].x(), target.bottom() + 1 - view->m_cornerTranslations[cbr].y(),
					target.left()      + view->m_cornerTranslations[ctl].x(), target.top()        + view->m_cornerTranslations[ctl].y(),
					target.right() + 1 - view->m_cornerTranslations[ctr].x(), target.top()        + view->m_cornerTranslations[ctr].y()
				};
				#endif
				

		//  		qDebug() << "GLWidget: vertexCoordArray: target: "<<target<<", points: "
		//  			<< "BL: "<<vertexCoordArray[0]<<vertexCoordArray[1]
		//  			<< "BR: "<<vertexCoordArray[2]<<vertexCoordArray[3]
		//  			<< "TL: "<<vertexCoordArray[4]<<vertexCoordArray[5]
		//  			<< "TR: "<<vertexCoordArray[6]<<vertexCoordArray[7];


				const GLfloat txLeft   = view->m_flipHorizontal ? source.right()  / m_fbo->size().width() : source.left()  / m_fbo->size().width();
				const GLfloat txRight  = view->m_flipHorizontal ? source.left()   / m_fbo->size().width() : source.right() / m_fbo->size().width();

				const GLfloat txTop    = view->m_flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
					? source.top()    / m_fbo->size().height()
					: source.bottom() / m_fbo->size().height();
				const GLfloat txBottom = view->m_flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
					? source.bottom() / m_fbo->size().height()
					: source.top()    / m_fbo->size().height();

				GLfloat textureCoordArray[] =
				{
					txLeft , txBottom,
					txRight, txBottom,
					txLeft , txTop,
					txRight, txTop
				};

		//  		qDebug() << "GLWidget: Before Rotate: textureCoordArray: "
		//  			<< "BL: "<<textureCoordArray[0]<<textureCoordArray[1]
		//  			<< "BR: "<<textureCoordArray[2]<<textureCoordArray[3]
		//  			<< "TL: "<<textureCoordArray[4]<<textureCoordArray[5]
		//  			<< "TR: "<<textureCoordArray[6]<<textureCoordArray[7];

				if(view->m_cornerRotation == GLRotateLeft)
				{
					// Bottom Left = Top Left
					textureCoordArray[0] = txLeft;
					textureCoordArray[1] = txTop;

					// Bottom Right = Bottom Left
					textureCoordArray[2] = txLeft;
					textureCoordArray[3] = txBottom;

					// Top Left = Top Right
					textureCoordArray[4] = txRight;
					textureCoordArray[5] = txTop;

					// Top Right = Bottom Right
					textureCoordArray[6] = txRight;
					textureCoordArray[7] = txBottom;
				}
				else
				if(view->m_cornerRotation == GLRotateRight)
				{
					// Bottom Left = Bottom Right
					textureCoordArray[0] = txRight;
					textureCoordArray[1] = txBottom;

					// Bottom Right = Top Right
					textureCoordArray[2] = txRight;
					textureCoordArray[3] = txTop;

					// Top Left = Bottom Left
					textureCoordArray[4] = txLeft;
					textureCoordArray[5] = txBottom;

					// Top Right = Top Left
					textureCoordArray[6] = txLeft;
					textureCoordArray[7] = txTop;
				}

		// 		qDebug() << "GLWidget: textureCoordArray: "
		//  			<< "BL: "<<textureCoordArray[0]<<textureCoordArray[1]
		//  			<< "BR: "<<textureCoordArray[2]<<textureCoordArray[3]
		//  			<< "TL: "<<textureCoordArray[4]<<textureCoordArray[5]
		//  			<< "TR: "<<textureCoordArray[6]<<textureCoordArray[7];

				m_program->bind();

				m_program->enableAttributeArray("vertexCoordArray");
				m_program->enableAttributeArray("textureCoordArray");

				m_program->setAttributeArray("vertexCoordArray",  vertexCoordArray,  2);
				m_program->setAttributeArray("textureCoordArray", textureCoordArray, 2);

				#ifdef OPENCV_ENABLED
				m_program->setUniformValue("warpMatrix",          view->m_warpMatrix);
				#endif

				m_program->setUniformValue("positionMatrix",      positionMatrix);
			// 	QMatrix4x4 mat4(
			// 		positionMatrix[0][0], positionMatrix[0][1], positionMatrix[0][2], positionMatrix[0][3],
			// 		positionMatrix[1][0], positionMatrix[1][1], positionMatrix[1][2], positionMatrix[1][3],
			// 		positionMatrix[2][0], positionMatrix[2][1], positionMatrix[2][2], positionMatrix[2][3],
			// 		positionMatrix[3][0], positionMatrix[3][1], positionMatrix[3][2], positionMatrix[3][3]
			// 		);
			// 	m_program->setUniformValue("positionMatrix",      mat4);

				//qDebug() << "GLVideoDrawable:paintGL():"<<this<<", rendering with opacity:"<<opacity();
				m_program->setUniformValue("alpha",               (GLfloat)m_opacity);
				m_program->setUniformValue("texOffsetX",          (GLfloat)0.);//m_invertedOffset.x());
				m_program->setUniformValue("texOffsetY",          (GLfloat)0.);//m_invertedOffset.y());

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_fbo->texture());

		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, view->m_alphaTextureId);

				glActiveTexture(GL_TEXTURE0);

				m_program->setUniformValue("texRgb", 0);
				m_program->setUniformValue("alphaMask", 1);

				m_program->setUniformValue("colorMatrix", view->m_colorMatrix);

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				//glDrawArrays(GL_QUAD_STRIP, 0, 4);


				m_program->release();
			}
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_fbo->texture());

			//glTranslatef(0.0f,0.0f,-3.42f);

			glBegin(GL_QUADS);
				// Front Face
		//              glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		//              glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		//              glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		//              glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);

				//QRectF rect(10,10,320,240);
				QRectF rect(0,0,width(),height());

				qreal
					vx1 = rect.left(),
					vx2 = rect.right(),
					vy1 = rect.bottom(),
					vy2 = rect.top();

		//                 glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1,vy1,  0.0f); // top left
		//                 glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2,vy1,  0.0f); // top right
		//                 glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2,vy2,  0.0f); // bottom right
		//                 glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1,vy2,  0.0f); // bottom left

				if(1)
				{
	// 				glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1 + m_cornerTranslations[3].x(),vy1 + m_cornerTranslations[3].y(),  0.0f); // bottom left // 3
	// 				glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2 - m_cornerTranslations[2].x(),vy1 + m_cornerTranslations[2].y(),  0.0f); // bottom right // 2
	// 				glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2 - m_cornerTranslations[1].x(),vy2 - m_cornerTranslations[1].y(),  0.0f); // top right  // 1
	// 				glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1 + m_cornerTranslations[0].x(),vy2 - m_cornerTranslations[0].y(),  0.0f); // top left // 0

					glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1,vy1,  0.0f); // bottom left // 3
					glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2,vy1,  0.0f); // bottom right // 2
					glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2,vy2,  0.0f); // top right  // 1
					glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1,vy2,  0.0f); // top left // 0
				}

				if(0)
				{
					qreal inc = fabs(vy2-vy1)/10.;
					qreal dx = fabs(vx2-vx1);
					qreal dy = fabs(vy2-vy1);
					qreal xf = inc/dx;
					qreal yf = inc/dy;
					//qDebug() << "params:"<<xf<<yf;
					for(qreal x=vx1; x<vx2; x+=inc)
					{
						qreal tx = x/dx;
						//qDebug() << "tx:"<<tx;
						for(qreal y=vy1; y>=vy2; y-=inc)
						{
							qreal ty = 1.-(y/dy);
							//qDebug() << "Y:"<<y;
							//qDebug() << "at:"<<x<<",y:"<<y;
		// 					glTexCoord2f(0.0f, 0.0f); glVertex3f(x,y,  0.0f); // bottom left
		// 					glTexCoord2f(1.0f, 0.0f); glVertex3f(x+inc,y,  0.0f); // bottom right
		// 					glTexCoord2f(1.0f, 1.0f); glVertex3f(x+inc,y+inc,  0.0f); // top right
		// 					glTexCoord2f(0.0f, 1.0f); glVertex3f(x,y+inc,  0.0f); // top left


		// 					glTexCoord2f(0.0f, 0.0f); glVertex3f(x,y+inc,  0.0f); // bottom left
		// 					glTexCoord2f(1.0f, 0.0f); glVertex3f(x+inc,y+inc,  0.0f); // bottom right
		// 					glTexCoord2f(1.0f, 1.0f); glVertex3f(x+inc,y,  0.0f); // top right
		// 					glTexCoord2f(0.0f, 1.0f); glVertex3f(x,y,  0.0f); // top left

							glTexCoord2f(tx, ty);
										glVertex3f(x,y+inc,  0.0f); // bottom left

							glTexCoord2f(tx+xf, ty);
										glVertex3f(x+inc,y+inc,  0.0f); // bottom right

							glTexCoord2f(tx+xf, ty+yf);
										glVertex3f(x+inc,y,  0.0f); // top right

							glTexCoord2f(tx, ty+yf);
										glVertex3f(x,y,  0.0f); // top left
						}
					}
				}

		//              glTexCoord2f(0,0); glVertex3f( 0, 0,0); //lo
		//              glTexCoord2f(0,1); glVertex3f(256, 0,0); //lu
		//              glTexCoord2f(1,1); glVertex3f(256, 256,0); //ru
		//              glTexCoord2f(1,0); glVertex3f( 0, 256,0); //ro
			glEnd();
		}

		//qDebug() << "GLWidget::paintGL(): FBO painting complete";


	// 	GLuint	texture[1]; // Storage For One Texture
	// 	QImage texOrig, texGL;
	// 	if ( !texOrig.load( "me2.jpg" ) )
	// 	//if ( !texOrig.load( "Pm5544.jpg" ) )
	// 	{
	// 		texOrig = QImage( 16, 16, QImage::Format_RGB32 );
	// 		texOrig.fill( Qt::green );
	// 	}
	//
	// 	// Setup inital texture
	// 	texGL = QGLWidget::convertToGLFormat( texOrig );
	// 	glGenTextures( 1, &texture[0] );
	// 	glBindTexture( GL_TEXTURE_2D, texture[0] );
	// 	glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	//
	// 	glEnable(GL_TEXTURE_2D);					// Enable Texture Mapping ( NEW )
	//
	// 	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//
	// 	glLoadIdentity(); // Reset The View
	// 	//glTranslatef(0.0f,0.0f,-3.42f);
	//
	// 	glBindTexture(GL_TEXTURE_2D, texture[0]);
	//
	//
	// 	glBegin(GL_QUADS);
	// 		// Front Face
	// // 		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
	// // 		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
	// // 		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
	// // 		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
	//
	// 		QRectF rect(0,0,764,572);
	//
	// 		qreal
	// 			vx1 = rect.left(),
	// 			vx2 = rect.right(),
	// 			vy1 = rect.bottom(),
	// 			vy2 = rect.top();
	//
	// 		glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1,vy1,  0.0f); // top left
	// 		glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2,vy1,  0.0f); // top right
	// 		glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2,vy2,  0.0f); // bottom right
	// 		glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1,vy2,  0.0f); // bottom left
	//
	// // 		glTexCoord2f(0,0); glVertex3f( 0, 0,0); //lo
	// // 		glTexCoord2f(0,1); glVertex3f(256, 0,0); //lu
	// // 		glTexCoord2f(1,1); glVertex3f(256, 256,0); //ru
	// // 		glTexCoord2f(1,0); glVertex3f( 0, 256,0); //ro
	// 	glEnd();
	}
//
	//QTimer::singleShot(0, this, SIGNAL(updated()));
	emit updated();

	//qDebug() << "GLWidget::paintGL: elapsed:"<<time.elapsed()<<"ms";
}


void GLWidget::setFlipHorizontal(bool value)
{
	defaultSubview()->setFlipHorizontal(value);
}

void GLWidget::setFlipVertical(bool value)
{
	defaultSubview()->setFlipVertical(value);
}

void GLWidgetSubview::setFlipHorizontal(bool value)
{
	m_flipHorizontal = value;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::setFlipVertical(bool value)
{
	m_flipVertical = value;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}


/*!
*/
void GLWidget::setBrightness(int brightness)
{
	defaultSubview()->setBrightness(brightness);
}

/*!
*/
void GLWidget::setContrast(int contrast)
{
	defaultSubview()->setContrast(contrast);
}

/*!
*/
void GLWidget::setHue(int hue)
{
	defaultSubview()->setHue(hue);
}

/*!
*/
void GLWidget::setSaturation(int saturation)
{
	defaultSubview()->setSaturation(saturation);
}

/*!
*/
void GLWidgetSubview::setBrightness(int brightness)
{
	m_brightness = brightness;
	m_colorsDirty = true;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

/*!
*/
void GLWidgetSubview::setContrast(int contrast)
{
	m_contrast = contrast;
	m_colorsDirty = true;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

/*!
*/
void GLWidgetSubview::setHue(int hue)
{
	m_hue = hue;
	m_colorsDirty = true;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

/*!
*/
void GLWidgetSubview::setSaturation(int saturation)
{
	m_saturation = saturation;
	m_colorsDirty = true;
	if(m_glw)
		m_glw->updateGL();
	emit changed(this);
}

void GLWidgetSubview::updateColors()
{
	//qDebug() << "GLWidget::updateColors: b:"<<brightness<<", c:"<<contrast<<", h:"<<hue<<", s:"<<saturation;

	const qreal b = m_brightness / 200.0;
	const qreal c = m_contrast / 200.0 + 1.0;
	const qreal h = m_hue / 200.0;
	const qreal s = m_saturation / 200.0 + 1.0;

	const qreal cosH = qCos(M_PI * h);
	const qreal sinH = qSin(M_PI * h);

	const qreal h11 = -0.4728 * cosH + 0.7954 * sinH + 1.4728;
	const qreal h21 = -0.9253 * cosH - 0.0118 * sinH + 0.9523;
	const qreal h31 =  0.4525 * cosH + 0.8072 * sinH - 0.4524;

	const qreal h12 =  1.4728 * cosH - 1.3728 * sinH - 1.4728;
	const qreal h22 =  1.9253 * cosH + 0.5891 * sinH - 0.9253;
	const qreal h32 = -0.4525 * cosH - 1.9619 * sinH + 0.4525;

	const qreal h13 =  1.4728 * cosH - 0.2181 * sinH - 1.4728;
	const qreal h23 =  0.9253 * cosH + 1.1665 * sinH - 0.9253;
	const qreal h33 =  0.5475 * cosH - 1.3846 * sinH + 0.4525;

	const qreal sr = (1.0 - s) * 0.3086;
	const qreal sg = (1.0 - s) * 0.6094;
	const qreal sb = (1.0 - s) * 0.0820;

	const qreal sr_s = sr + s;
	const qreal sg_s = sg + s;
	const qreal sb_s = sr + s;

	const float m4 = (s + sr + sg + sb) * (0.5 - 0.5 * c + b);

	m_colorMatrix(0, 0) = c * (sr_s * h11 + sg * h21 + sb * h31);
	m_colorMatrix(0, 1) = c * (sr_s * h12 + sg * h22 + sb * h32);
	m_colorMatrix(0, 2) = c * (sr_s * h13 + sg * h23 + sb * h33);
	m_colorMatrix(0, 3) = m4;

	m_colorMatrix(1, 0) = c * (sr * h11 + sg_s * h21 + sb * h31);
	m_colorMatrix(1, 1) = c * (sr * h12 + sg_s * h22 + sb * h32);
	m_colorMatrix(1, 2) = c * (sr * h13 + sg_s * h23 + sb * h33);
	m_colorMatrix(1, 3) = m4;

	m_colorMatrix(2, 0) = c * (sr * h11 + sg * h21 + sb_s * h31);
	m_colorMatrix(2, 1) = c * (sr * h12 + sg * h22 + sb_s * h32);
	m_colorMatrix(2, 2) = c * (sr * h13 + sg * h23 + sb_s * h33);
	m_colorMatrix(2, 3) = m4;

	m_colorMatrix(3, 0) = 0.0;
	m_colorMatrix(3, 1) = 0.0;
	m_colorMatrix(3, 2) = 0.0;
	m_colorMatrix(3, 3) = 1.0;
}

void GLWidget::addDrawable(GLDrawable *item)
{
	//makeCurrentIfNeeded();

// 	QString newName = QString("%1/%2").arg(objectName()).arg(item->objectName());
// 	item->setObjectName(qPrintable(newName));
	if(m_drawables.contains(item))
	{
		qDebug() << "GLWidget::addDrawable: "<<(QObject*)item<<" already on display, not re-adding.";
		return;
	}

	item->setGLWidget(this);
	m_drawables << item;
	connect(item, SIGNAL(zIndexChanged(double)), this, SLOT(zIndexChanged()));
	if(m_glInited)
	{
		//qDebug() << "GLWidget::addDrawable()";
		item->initGL();
	}
	sortDrawables();

	updateGL();
}

void GLWidget::removeDrawable(GLDrawable *item)
{
// 	qDebug() << "GLWidget::removeDrawable(): drawable:"<<((void*)item);
	m_drawables.removeAll(item);
	item->setGLWidget(0);
	disconnect(item, 0, this, 0);
	// sort not needed since order implicitly stays the same
	updateGL();
}

void GLWidget::zIndexChanged()
{
	sortDrawables();
}

bool GLWidget_drawable_zIndex_compare(GLDrawable *a, GLDrawable *b)
{
	return (a && b) ? a->zIndex() < b->zIndex() : true;
}


void GLWidget::sortDrawables()
{
	qSort(m_drawables.begin(), m_drawables.end(), GLWidget_drawable_zIndex_compare);
}

void GLWidget::resizeGL(int width, int height)
{
	makeCurrentIfNeeded();
	
	glViewport(0,0,width,height); //(width - side) / 2, (height - side) / 2, side, side);

	//qDebug() << "GLWidget::resizeGL(): "<<windowTitle()<<" width:"<<width<<", height:"<<height;

	if(m_fboEnabled &&
		(!m_fbo ||
			(m_fbo &&
				(m_fbo->size().width()  != width ||
				 m_fbo->size().height() != height)
			)
		)
	  )
	{
		makeCurrentIfNeeded();

		if(m_fbo)
			delete m_fbo;

		QSize size(width,height);
		m_fbo = new QGLFramebufferObject(size);

		//qDebug() << "GLWidget::resizeGL(): New FBO size:"<<m_fbo->size();
	}

	if(height == 0)
		height = 1;

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	//gluPerspective(45.0f,(GLfloat)w/(GLfloat)h,0.1f,100.0f);
	glOrtho(0, width, height, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix

	setupReadbackBuffers();
	setupReadbackFBO();
	
	//qDebug() << "GLWidget::resizeGL: "<<width<<","<<height;
	setViewport(viewport());

	foreach(GLWidgetSubview *view, m_subviews)
	{
		view->setAlphaMask(view->m_alphaMask_preScaled);
		view->updateWarpMatrix();
	}
}

void GLWidget::setOutputSize(QSize size)
{
	if(size.isValid())
	{
		m_readbackSize = size;
		m_readbackSizeAuto = false;
	}
	else
	{
		m_readbackSizeAuto = true;
	}

	setupReadbackBuffers();
	setupReadbackFBO();
}


void GLWidget::setupReadbackBuffers()
{
	if(m_readbackSizeAuto)
		m_readbackSize = size();
		
	if(m_readbackTextureSize != size())
	{
		m_readbackTextureSize = size();
		
		makeCurrentIfNeeded();
			
		glGenTextures(1, &m_readbackTextureId);
		glBindTexture(GL_TEXTURE_2D, m_readbackTextureId);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			//GL_RGBA8,
			//GL_BGRA,
			GL_BGR,
			width(),
			height(),
			0,
			GL_RGB, //_EXT,
			GL_UNSIGNED_BYTE,
			NULL
			);
		
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
				
			
// 		glTexImage2D(GL_TEXTURE_2D,
// 			0, GL_RGB,
// 			width(),
// 			height(),
// 			0, GL_RGB, 
// 			GL_UNSIGNED_BYTE,
// 			NULL);
	}
	
		
	if(m_pboSize != m_readbackSize)
	{
		m_pboSize = m_readbackSize;
		
		makeCurrentIfNeeded();
			
		#ifndef Q_OS_WIN32
		if(m_pboEnabled)
		{
			// 4 = channel count (RGBA)
			int dataSize = m_readbackSize.width() * m_readbackSize.height() * 4;

			// create 2 pixel buffer objects, you need to delete them when program exits.
			// glBufferDataARB with NULL pointer reserves only memory space.
			glGenBuffersARB(2, m_pboIds);
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pboIds[0]);
			glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, dataSize, 0, GL_STREAM_READ_ARB);
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pboIds[1]);
			glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, dataSize, 0, GL_STREAM_READ_ARB);

			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

			//qDebug() << "GLWidget::setupReadbackBuffers(): Generated FBO size:"<<m_readbackSize<<", PBO data size: "<<dataSize/1024<<" Kb";
		}
		#endif
	}

}

void GLWidget::setupReadbackFBO()
{
	if(!m_readbackFbo || 
	    m_readbackSize != m_readbackFbo->size())
	{
		// Setup readback FBO
		if(m_readbackFbo)
			delete m_readbackFbo;

		m_readbackFbo = new QGLFramebufferObject(m_readbackSize);
	}
}

void GLWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
	//qDebug() << "GLVideoDrawable::setAspectRatioMode: "<<this<<", mode:"<<mode<<", int:"<<(int)mode;
	m_aspectRatioMode = mode;
	setViewport(viewport());
	updateGL();
}


void GLWidget::setViewport(const QRectF& rect)
{
	m_viewport = rect;

	QRectF viewport = m_viewport;
	if(!viewport.isValid())
	{
		QSizeF canvas = m_canvasSize;
		if(canvas.isNull() || !canvas.isValid())
			canvas = QSizeF(1000.,750.);
		viewport = QRectF(QPointF(0.,0.),canvas);
	}

	//qDebug() << "GLWidget::setViewport: "<<viewport;

	float vw = viewport.width();
	float vh = viewport.height();

	// Scale viewport size to our size
	float winWidth  = (float)(m_fbo ? m_fbo->size().width()  : width());
	float winHeight = (float)(m_fbo ? m_fbo->size().height() : height());

	float sx = winWidth  / vw;
	float sy = winHeight / vh;

	//qDebug() << "

	//float scale = qMin(sx,sy);
	if (m_aspectRatioMode != Qt::IgnoreAspectRatio)
	{
		if(sx < sy)
			sy = sx;
		else
			sx = sy;
	}

	// Center viewport in our rectangle
	float scaledWidth  = vw * sx;//scale;
	float scaledHeight = vh * sy;//scale;

	// Calculate centering
	float xt = (winWidth  - scaledWidth) /2;
	float yt = (winHeight - scaledHeight)/2;

	// Apply top-left translation for viewport location
	float xtv = xt - viewport.left() * sx;//scale;
	float ytv = yt - viewport.top()  * sy;//scale;

	setTransform(QTransform().translate(xtv,ytv).scale(sx,sy));//scale,scale));

	//QSize size(width,height);
	QSize size = viewport.size().toSize();
	//qDebug() <<"GLWidget::setViewport: "<<windowTitle()<<" size:"<<size<<", sx:"<<sx<<",sy:"<<sy<<", xtv:"<<xtv<<", ytv:"<<ytv<<", scaled size:"<<scaledWidth<<scaledHeight;
	foreach(GLDrawable *drawable, m_drawables)
		drawable->viewportResized(size);

	updateGL();

/*#if !defined(QT_OPENGL_ES_2)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
		#ifndef QT_OPENGL_ES
			glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
		#else
			glOrthof(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
		#endif
	glMatrixMode(GL_MODELVIEW);
#endif*/
}

void GLWidget::setCanvasSize(const QSizeF& size)
{
	m_canvasSize = size;
	foreach(GLDrawable *drawable, m_drawables)
		drawable->canvasResized(size);
}

void GLWidget::setTransform(const QTransform& tx)
{
	m_transform = tx;
}

void GLWidget::mousePressEvent(QMouseEvent */*event*/)
{
// 	lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent */*event*/)
{
// 	int dx = event->x() - lastPos.x();
// 	int dy = event->y() - lastPos.y();
//
// 	if (event->buttons() & Qt::LeftButton) {
// 		rotateBy(8 * dy, 8 * dx, 0);
// 	} else if (event->buttons() & Qt::RightButton) {
// 		rotateBy(8 * dy, 0, 8 * dx);
// 	}
// 	lastPos = event->pos();
}

void GLWidget::mouseReleaseEvent(QMouseEvent * /* event */)
{
	emit clicked();
}

void GLWidget::setOpacity(double d)
{
	m_opacity = d;
	//qDebug() << "GLWidget::setOpacity: "<<d;

	updateGL();
}

void GLWidget::fadeBlack(bool toBlack)
{
	//qDebug() << "GLWidget::fadeBlack: toBlack:"<<toBlack<<", duration:"<<m_crossfadeSpeed<<", current opac:"<<opacity();

	m_fadeBlackTimer.setInterval(1000 / 30); // 30fps fade

	m_fadeBlackDirection = toBlack ?  -1 : 1;

	m_fadeBlackClock.start();
	m_fadeBlackTimer.start();

	m_isBlack = toBlack;
}

void GLWidget::fadeBlackTick()
{
	int time = m_fadeBlackClock.elapsed();
	if(time >= m_crossfadeSpeed)
	{
		m_fadeBlackTimer.stop();
		setOpacity(m_fadeBlackDirection > 0 ? 1 : 0);
	}
	else
	{
		double fadeVal = ((double)time) / ((double)m_crossfadeSpeed);
		if(m_fadeBlackDirection < 0)
			fadeVal = 1.0 - fadeVal;
		//qDebug() << "GLWidget::fadeBlackTick: dir:"<<m_fadeBlackDirection<<", time:"<<time<<", len:"<<m_crossfadeSpeed<<", fadeVal:"<<fadeVal;
		setOpacity(fadeVal);
	}
}


void GLWidget::setCrossfadeSpeed(int m)
{
	m_crossfadeSpeed = m;
}


GLWidgetOutputStream *GLWidget::outputStream()
{
	if(!m_outputStream)
	{
		m_outputStream = new GLWidgetOutputStream(this);
		m_outputStream->start();
	}
	return m_outputStream;
}

QImage GLWidget::toImage()
{
	if(m_fbo)
		return m_fbo->toImage();
	else
		return grabFrameBuffer();
}

void GLWidget::setFboEnabled(bool flag)
{
	qDebug() << "GLWidget::setFboEnabled: flag:"<<flag;
	m_fboEnabled = flag;
	if(!flag && m_fbo)
	{
		delete m_fbo;
		m_fbo = 0;
	}

	// resizeGL() will re-create m_fbo as needed
	resizeGL(width(),height());
	updateGL();
}


void GLWidget::setBackgroundColor(QColor bg)
{
	m_backgroundColor = bg;
	updateGL();
}

/// GLWidgetOutputStream

GLWidgetOutputStream::GLWidgetOutputStream(GLWidget *parent)
	: VideoSource(parent)
	, m_glWidget(parent)
	, m_fps(35)
	, m_frameUpdated(false)
{
	m_data.clear(); /// IS this needed?
	
	setAutoDestroy(false);
 	setIsBuffered(false);
	setImage(QImage("dot.gif"));
	connect(&m_frameReadyTimer, SIGNAL(timeout()), this, SIGNAL(frameReady()));
	m_frameReadyTimer.setSingleShot(true);
	setFps(m_fps);
}

void GLWidgetOutputStream::setImage(QImage img)
{
	QMutexLocker lock(&m_dataMutex);
	//m_image = img.copy();
	//m_image = QImage(img.size(), img.format());
	//memcpy(m_image.bits(), img.bits(), img.byteCount());
	m_format = img.format();
	m_size = img.size();
	uchar *ptr = (uchar*)malloc(sizeof(uchar) * img.byteCount());
	memcpy((uchar*)ptr, img.bits(), img.byteCount());

	//m_data = ptr;
	m_data = QSharedPointer<uchar>(ptr);

	//qDebug() << "GLWidgetOutputStream::setImage(): Received frame buffer, size:"<<m_image.size()<<", img format:"<<m_image.format();
	//enqueue(new VideoFrame(m_image,1000/m_fps, QTime::currentTime()));

// 	if(!m_frameReadyTimer.isActive())
// 		m_frameReadyTimer.start();
	m_frameUpdated = true;
}


// By allowing GLWidget to pass in the pointer directly instead of
// first copying to a QImage, we can save a memcpy()
void GLWidgetOutputStream::copyPtr(GLubyte *ptrIn, QSize size)
{
	QMutexLocker lock(&m_dataMutex);

	m_format = QImage::Format_ARGB32;
	m_size = size;

	int bytes = m_size.width() * m_size.height() * 4;
	uchar *ptr = (uchar*)malloc(sizeof(uchar) * bytes);
	memcpy((uchar*)ptr, ptrIn, bytes);

	m_data = QSharedPointer<uchar>(ptr);
	m_stamp = QTime::currentTime();
	m_frameUpdated = true;
}

void GLWidgetOutputStream::run()
{
	//int count = 0;
	while(!m_killed)
	{
 		if(m_frameUpdated)
 		{
			QMutexLocker lock(&m_dataMutex);
			if(m_data)
			{
				QImage img((uchar*)m_data.data(), m_size.width(), m_size.height(), m_format);
				img = img.mirrored();

				VideoFrame *frame = new VideoFrame(img,1000/m_fps);
				frame->setCaptureTime(m_stamp);
				enqueue(frame);
				m_frameUpdated = false;
				emit frameReady();
				//qDebug() << "GLWidgetOutputStream::run(): new frame, size: "<<img.byteCount()/1024<<" Kb";

				/*count ++;
				QString file = QString("debug/output-%1.png").arg(count % 2 == 0 ? 0 : 1);
				img.save(file);*/
			}
		}
		msleep(1000/m_fps);
	}
}


void GLWidgetOutputStream::setFps(int fps)
{
	m_fps = fps;
	m_frameReadyTimer.setInterval(1000/m_fps);
}
