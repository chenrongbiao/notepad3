﻿// This module implements the QsciLexer class.
//
// Copyright (c) 2023 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of QScintilla.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include "Qsci/qscilexer.h"

#include <qapplication.h>
#include <qcolor.h>
#include <qfont.h>
#include <qsettings.h>

#include "Qsci/qsciapis.h"
#include "Qsci/qsciscintilla.h"
#include "Qsci/qsciscintillabase.h"

int QsciLexer::s_defaultFontSize = 12;

int QsciLexer::m_themesId = 0;

#if defined(Q_OS_WIN)
QFont QsciLexer::s_defaultLangFont("Courier New", QsciLexer::s_defaultFontSize);
#elif defined(Q_OS_MAC)
QFont QsciLexer::s_defaultLangFont("Menlo", s_defaultFontSize);
#else
QFont QsciLexer::s_defaultLangFont("Courier 10 Pitch", 12);
#endif

// The ctor.
QsciLexer::QsciLexer(QObject *parent)
    : QObject(parent),
      autoIndStyle(-1), apiSet(0), attached_editor(0), m_lexerId(QsciScintillaBase::SCLEX_CONTAINER), m_isUserDefineKeyword(false)
{
#if 0
#if defined(Q_OS_WIN)
    defFont = QFont("Courier New", s_defaultFontSize);
#elif defined(Q_OS_MAC)
    defFont = QFont("Menlo", s_defaultFontSize);
#else
    defFont = QFont("Bitstream Vera Sans", 9);
#endif
#endif
	defFont = s_defaultLangFont;

    // Set the default fore and background colours.
    QPalette pal = QApplication::palette();
    //defColor = pal.text().color();
	defColor = QColor(Qt::black);
    defPaper = pal.base().color();

    // Putting this on the heap means we can keep the style getters const.
    style_map = new StyleDataMap;
    style_map->style_data_set = false;
}


// The dtor.
QsciLexer::~QsciLexer()
{
    delete style_map;
}

void QsciLexer::setProLangeDefaultFont(const QFont & font)
{
	s_defaultLangFont = font;
}


// Set the attached editor.
void QsciLexer::setEditor(QsciScintilla *editor)
{
    attached_editor = editor;
}


// Return the lexer name.
const char *QsciLexer::lexer() const
{
    return 0;
}

void  QsciLexer::setLexerTag(QString tag)
{
	m_tagName = tag;
}

QString QsciLexer::lexerTag()
{
	if (m_tagName.isEmpty())
	{
		return QString(lexer());
	}
	return m_tagName;
}


// Return the lexer identifier.
int QsciLexer::lexerId() const
{
	return m_lexerId;
}

void QsciLexer::setLexerId(int id)
{
	m_lexerId = id;
}


// Return the number of style bits needed by the lexer.
int QsciLexer::styleBitsNeeded() const
{
    return 8;
}


// Make sure the style defaults have been set.
void QsciLexer::setStyleDefaults() const
{
    if (!style_map->style_data_set)
    {
        for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
        {
            if (!description(i).isEmpty())
            {
                setThemesDefaultStyleData(i);
            }
        }

        style_map->style_data_set = true;
    }
}

//恢复到默认lexer状态
void QsciLexer::resetStyleDefaults()
{
	style_map->style_data_set = false;
	style_map->style_data.clear();
	setStyleDefaults();
}


// Return a reference to a style's data, setting up the defaults if needed.
QsciLexer::StyleData &QsciLexer::styleData(int style) const
{
    StyleData &sd = style_map->style_data[style];

    //如果是非默认主题,则无条件的把所有属性都设置为默认值
    //这样一来，默认都是默认风格，只有文件中存在配置值的才是其它指定风格
 
    // See if this is a new style by checking if the colour is valid.
    if (!sd.color.isValid())
    {
        sd.color = defaultColor(style);
        sd.paper = defaultPaper(style);
        sd.font = defaultFont(style);
        sd.eol_fill = defaultEolFill(style);
    }
   
    return sd;
}

// Return a reference to a style's data, setting up the defaults if needed.
QsciLexer::StyleData& QsciLexer::setThemesDefaultStyleData(int style) const
{

    StyleData& sd = style_map->style_data[style];

    //如果是非默认主题,则无条件的把所有属性都设置为默认值
    //这样一来，默认都是GLobal的默认风格，只有文件中存在配置值的才是其它指定风格
    //外部已经使用Global的默认值，填充了QsciLexer的默认值
    if (L_GLOBAL != lexerId())
    {
        // See if this is a new style by checking if the colour is valid.
        //20230209 默认风格，不能全部使用默认值，使用风格本身值，否则默认风格丢失
        //20230209 发现当默认重置，切换到黑色，再切换到默认，默认丢失颜色高亮。
        //因为语法其实没有使用默认赋值。
        if (m_themesId == 0)
        {
            sd.color = defaultColor(style);
            sd.paper = defaultPaper(style);
            sd.font = defaultFont(style);
            sd.eol_fill = defaultEolFill(style);
        }
        else if (m_themesId != 0 || !sd.color.isValid())
        {
            sd.color = defaultColor();
            sd.paper = defaultPaper();
            sd.font = defaultFont();
            sd.eol_fill = defaultEolFill(style);
        }
    }
    else
    {
        //Global本身是个例外，它不能全部使用默认风格。而是要使用它自定义的风格。
        if (m_themesId != 0 || !sd.color.isValid())
        {
            sd.color = defaultColor(style);
            sd.paper = defaultPaper(style);
            sd.font = defaultFont(style);
            sd.eol_fill = defaultEolFill(style);
        }
    }

    return sd;
}

// Set the APIs associated with the lexer.
void QsciLexer::setAPIs(QsciAbstractAPIs *apis)
{
    apiSet = apis;
}


// Return a pointer to the current APIs if there are any.
QsciAbstractAPIs *QsciLexer::apis() const
{
    return apiSet;
}


// Default implementation to return the set of fill up characters that can end
// auto-completion.
const char *QsciLexer::autoCompletionFillups() const
{
    return "(";
}


// Default implementation to return the view used for indentation guides.
int QsciLexer::indentationGuideView() const
{
    return QsciScintillaBase::SC_IV_LOOKBOTH;
}


// Default implementation to return the list of character sequences that can
// separate auto-completion words.
QStringList QsciLexer::autoCompletionWordSeparators() const
{
    return QStringList();
}


// Default implementation to return the list of keywords that can start a
// block.
const char *QsciLexer::blockStartKeyword(int *) const
{
    return 0;
}


// Default implementation to return the list of characters that can start a
// block.
const char *QsciLexer::blockStart(int *) const
{
    return 0;
}


// Default implementation to return the list of characters that can end a
// block.
const char *QsciLexer::blockEnd(int *) const
{
    return 0;
}


// Default implementation to return the style used for braces.
int QsciLexer::braceStyle() const
{
    return -1;
}


// Default implementation to return the number of lines to look back when
// auto-indenting.
int QsciLexer::blockLookback() const
{
    return 20;
}


// Default implementation to return the case sensitivity of the language.
bool QsciLexer::caseSensitive() const
{
    return true;
}


// Default implementation to return the characters that make up a word.
const char *QsciLexer::wordCharacters() const
{
    return 0;
}


// Default implementation to return the style used for whitespace.
int QsciLexer::defaultStyle() const
{
    return 0;
}


// Returns the foreground colour of the text for a style.
QColor QsciLexer::color(int style) const
{
    return styleData(style).color;
}


// Returns the background colour of the text for a style.
QColor QsciLexer::paper(int style) const
{
    return styleData(style).paper;
}


// Returns the font for a style.
QFont QsciLexer::font(int style) const
{
    return styleData(style).font;
}


// Returns the end-of-line fill for a style.
bool QsciLexer::eolFill(int style) const
{
    return styleData(style).eol_fill;
}


// Returns the set of keywords.
const char *QsciLexer::keywords(int)
{
	if (m_isUserDefineKeyword)
	{
		//如果是自定义用户关键字，则根据语言tag获取
		return getUserDefineKeywords();
	}

    return 0;
}


// Returns the default EOL fill for a style.
bool QsciLexer::defaultEolFill(int) const
{
    return false;
}


// Returns the default font for a style.
QFont QsciLexer::defaultFont(int) const
{
    return defaultFont();
}


// Returns the default font.
QFont QsciLexer::defaultFont() const
{
    return defFont;
}


// Sets the default font.
void QsciLexer::setDefaultFont(const QFont &f)
{
    defFont = f;
}


// Returns the default text colour for a style.
QColor QsciLexer::defaultColor(int) const
{
    return defaultColor();
}


// Returns the default text colour.
QColor QsciLexer::defaultColor() const
{
    return defColor;
}


// Sets the default text colour.
void QsciLexer::setDefaultColor(const QColor &c)
{
    defColor = c;
}


// Returns the default paper colour for a styles.
QColor QsciLexer::defaultPaper(int) const
{
    return defaultPaper();
}


// Returns the default paper colour.
QColor QsciLexer::defaultPaper() const
{
    return defPaper;
}


// Sets the default paper colour.
void QsciLexer::setDefaultPaper(const QColor &c)
{
    defPaper = c;

    // Normally the default values are only intended to provide defaults when a
    // lexer is first setup because once a style has been referenced then a
    // copy of the default is made.  However the default paper is a special
    // case because there is no other way to set the background colour used
    // where there is no text.  Therefore we also actively set it.
    setPaper(c, QsciScintillaBase::STYLE_DEFAULT);
}


// Read properties from the settings.
bool QsciLexer::readProperties(QSettings &,const QString &)
{
    return true;
}


// Refresh all properties.
void QsciLexer::refreshProperties()
{
}


// Write properties to the settings.
bool QsciLexer::writeProperties(QSettings &,const QString &) const
{
    return true;
}


// Restore the user settings.
bool QsciLexer::readSettings(QSettings &qs,const char *prefix)
{
    bool ok, flag, rc = true;
    int num;
    QString key, full_key;
    QStringList fdesc;

    //原来是先读取默认值，在读取配置值。加入主题后，得先读取配置的默认值。
    //因为非默认主题的初始样式值，就该是默认样式值。只有读取配置的默认值，后面初始化样式时，
    //才能获取到真正的样式默认值。
    if (m_themesId != 0)
    {
        key = QString("%1/%2/").arg(prefix).arg(language());

        // Read the default foreground colour.
        full_key = key + "defaultcolor";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok, 16);

        if (ok)
            setDefaultColor(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
        else
            rc = false;

        // Read the default background colour.
        full_key = key + "defaultpaper";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok, 16);

        if (ok)
            setDefaultPaper(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
        else
            rc = false;

        // Read the default font.  First try the deprecated format that uses an
        // integer point size.
        full_key = key + "defaultfont";

        ok = qs.contains(full_key);
        fdesc = qs.value(full_key).toStringList();

        if (ok && fdesc.count() == 5)
        {
            QFont f;

            f.setFamily(fdesc[0]);
            f.setPointSize(fdesc[1].toInt());
            f.setBold(fdesc[2].toInt());
            f.setItalic(fdesc[3].toInt());
            f.setUnderline(fdesc[4].toInt());

            setDefaultFont(f);
        }
        else
            rc = false;
    }

    setStyleDefaults();

    // Read the styles.
    for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
    {
        // Ignore invalid styles.
        if (description(i).isEmpty())
            continue;

        key = QString("%1/%2/style%3/").arg(prefix).arg(language()).arg(i);

        // Read the foreground colour.
        full_key = key + "color";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok, 16);

        if (ok)
            setColor(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff), i);
        else
            rc = false;

        // Read the end-of-line fill.
        full_key = key + "eolfill";

        ok = qs.contains(full_key);
        flag = qs.value(full_key, false).toBool();

        if (ok)
            setEolFill(flag, i);
        else
            rc = false;

        // Read the font.  First try the deprecated format that uses an integer
        // point size.
        full_key = key + "font";

        ok = qs.contains(full_key);
        fdesc = qs.value(full_key).toStringList();

        if (ok && fdesc.count() == 5)
        {
            QFont f;

            f.setFamily(fdesc[0]);
            f.setPointSize(fdesc[1].toInt());
            f.setBold(fdesc[2].toInt());
            f.setItalic(fdesc[3].toInt());
            f.setUnderline(fdesc[4].toInt());

            setFont(f, i);
        }
        else
            rc = false;

        // Now try the newer font format that uses a floating point point size.
        // It is not an error if it doesn't exist.
        //full_key = key + "font2";

       // ok = qs.contains(full_key);
       // fdesc = qs.value(full_key).toStringList();

        /*if (ok)
        {
             Allow for future versions with more fields.
            if (fdesc.count() >= 5)
            {
                QFont f;

                f.setFamily(fdesc[0]);
                f.setPointSizeF(fdesc[1].toDouble());
                f.setBold(fdesc[2].toInt());
                f.setItalic(fdesc[3].toInt());
                f.setUnderline(fdesc[4].toInt());

                setFont(f, i);
            }
            else
            {
                rc = false;
            }
        }*/

#if 1 //不读取背景颜色，和主题保存一致
        // Read the background colour.
        full_key = key + "paper";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok,16);

        if (ok)
            setPaper(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff), i);
        else
            rc = false;
#endif
    }

    // Read the properties.
    key = QString("%1/%2/properties/").arg(prefix).arg(language());

    if (!readProperties(qs,key))
        rc = false;

    refreshProperties();

    //只有默认主题才需要读取默认值。非默认主题，最前面已经读取过了。
    if (m_themesId == 0)
    {
        // Read the rest.
        key = QString("%1/%2/").arg(prefix).arg(language());

        // Read the default foreground colour.
        full_key = key + "defaultcolor";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok, 16);

        if (ok)
            setDefaultColor(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
        else
            rc = false;

        // Read the default background colour.
        full_key = key + "defaultpaper";

        ok = qs.contains(full_key);
        num = qs.value(full_key).toString().toInt(&ok, 16);

        if (ok)
            setDefaultPaper(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
        else
            rc = false;

        // Read the default font.  First try the deprecated format that uses an
        // integer point size.
        full_key = key + "defaultfont";

        ok = qs.contains(full_key);
        fdesc = qs.value(full_key).toStringList();

        if (ok && fdesc.count() == 5)
        {
            QFont f;

            f.setFamily(fdesc[0]);
            f.setPointSize(fdesc[1].toInt());
            f.setBold(fdesc[2].toInt());
            f.setItalic(fdesc[3].toInt());
            f.setUnderline(fdesc[4].toInt());

            setDefaultFont(f);
        }
        else
            rc = false;
    }

    // Now try the newer font format that uses a floating point point size.  It
    // is not an error if it doesn't exist.
    //full_key = key + "defaultfont2";

    //ok = qs.contains(full_key);
    //fdesc = qs.value(full_key).toStringList();

    //if (ok)
    //{
    //    // Allow for future versions with more fields.
    //    if (fdesc.count() >= 5)
    //    {
    //        QFont f;

    //        f.setFamily(fdesc[0]);
    //        f.setPointSizeF(fdesc[1].toDouble());
    //        f.setBold(fdesc[2].toInt());
    //        f.setItalic(fdesc[3].toInt());
    //        f.setUnderline(fdesc[4].toInt());

    //        setDefaultFont(f);
    //    }
    //    else
    //    {
    //        rc = false;
    //    }
    //}

    full_key = key + "autoindentstyle";

    ok = qs.contains(full_key);
    num = qs.value(full_key).toInt();

    if (ok)
        setAutoIndentStyle(num);
    else
        rc = false;

    return rc;
}


// Save the user settings.
bool QsciLexer::writeSettings(QSettings &qs,const char *prefix) const
{
    bool rc = true;
    QString key, fmt("%1");
    int num;
    QStringList fdesc;

    setStyleDefaults();

    // Write the styles.
    for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
    {
        // Ignore invalid styles.
        if (description(i).isEmpty())
            continue;

        QColor c;

        key = QString("%1/%2/style%3/").arg(prefix).arg(language()).arg(i);

        // Write the foreground colour.
        c = color(i);
        num = (c.red() << 16) | (c.green() << 8) | c.blue();

        qs.setValue(key + "color", QString::number(num, 16));

        // Write the end-of-line fill.
        qs.setValue(key + "eolfill", eolFill(i));

        // Write the font using the deprecated format.
        QFont f = font(i);

        fdesc.clear();
        fdesc += f.family();
        fdesc += fmt.arg(f.pointSize());

        // The casts are for Borland.
        fdesc += fmt.arg((int)f.bold());
        fdesc += fmt.arg((int)f.italic());
        fdesc += fmt.arg((int)f.underline());

        qs.setValue(key + "font", fdesc);

        // Write the font using the newer format.
        //fdesc[1] = fmt.arg(f.pointSizeF());

        //qs.setValue(key + "font2", fdesc);

#if 1 //背景颜色和主题皮肤保存一致，故不写入背景颜色
        // Write the background colour.
        c = paper(i);
        num = (c.red() << 16) | (c.green() << 8) | c.blue();

        qs.setValue(key + "paper", QString::number(num, 16));
#endif
    }

    // Write the properties.
    key = QString("%1/%2/properties/").arg(prefix).arg(language());

    if (!writeProperties(qs,key))
        rc = false;

    // Write the rest.
    key = QString("%1/%2/").arg(prefix).arg(language());

    // Write the default foreground colour.
    num = (defColor.red() << 16) | (defColor.green() << 8) | defColor.blue();

    qs.setValue(key + "defaultcolor", QString::number(num,16));

#if 1
    // Write the default background colour.
    num = (defPaper.red() << 16) | (defPaper.green() << 8) | defPaper.blue();

    qs.setValue(key + "defaultpaper", QString::number(num, 16));
#endif

    // Write the default font using the deprecated format.
    fdesc.clear();
    fdesc += defFont.family();
    fdesc += fmt.arg(defFont.pointSize());

    // The casts are for Borland.
    fdesc += fmt.arg((int)defFont.bold());
    fdesc += fmt.arg((int)defFont.italic());
    fdesc += fmt.arg((int)defFont.underline());

    qs.setValue(key + "defaultfont", fdesc);

    // Write the font using the newer format.
    //fdesc[1] = fmt.arg(defFont.pointSizeF());

   //qs.setValue(key + "defaultfont2", fdesc);

    qs.setValue(key + "autoindentstyle", autoIndStyle);

    return rc;
}


// Return the auto-indentation style.
int QsciLexer::autoIndentStyle()
{
    // We can't do this in the ctor because we want the virtuals to work.
    if (autoIndStyle < 0)
        autoIndStyle = (blockStartKeyword() || blockStart() || blockEnd()) ?
                    0 : QsciScintilla::AiMaintain;

    return autoIndStyle;
}


// Set the auto-indentation style.
void QsciLexer::setAutoIndentStyle(int autoindentstyle)
{
    autoIndStyle = autoindentstyle;
}


// Set the foreground colour for a style.
void QsciLexer::setColor(const QColor &c, int style)
{
    if (style >= 0)
    {
        styleData(style).color = c;
        emit colorChanged(c, style);
    }
    else
        for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
            if (!description(i).isEmpty())
                setColor(c, i);
}


// Set the end-of-line fill for a style.
void QsciLexer::setEolFill(bool eolfill, int style)
{
    if (style >= 0)
    {
        styleData(style).eol_fill = eolfill;
        emit eolFillChanged(eolfill, style);
    }
    else
        for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
            if (!description(i).isEmpty())
                setEolFill(eolfill, i);
}


// Set the font for a style.
void QsciLexer::setFont(const QFont &f, int style)
{
    if (style >= 0)
    {
        styleData(style).font = f;
        emit fontChanged(f, style);
    }
    else
        for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
            if (!description(i).isEmpty())
                setFont(f, i);
}


// Set the background colour for a style.
void QsciLexer::setPaper(const QColor &c, int style)
{
    if (style >= 0)
    {
        styleData(style).paper = c;
        emit paperChanged(c, style);
    }
    else
    {
        for (int i = 0; i <= QsciScintillaBase::STYLE_MAX; ++i)
            if (!description(i).isEmpty())
                setPaper(c, i);

        emit paperChanged(c, QsciScintillaBase::STYLE_DEFAULT);
    }
}

void  QsciLexer::setIsUserDefineKeywords(bool isUserDefine)
{
	m_isUserDefineKeyword = isUserDefine;
}


const char* QsciLexer::getUserDefineKeywords()
{

	QString userLangFile = QString("notepad/userlang/%1").arg(m_tagName);//自定义语言中不能有.字符，否则可能有错，后续要检查
	QSettings qs(QSettings::IniFormat, QSettings::UserScope, userLangFile);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	qs.setIniCodec("UTF-8");
#endif

	if (!qs.contains("mz"))
	{
		return nullptr;
	}

	//自定义语言格式。
	//mz:ndd
	//name:xxx
	//mother:xxx none/cpp/html 就三种
	//ext:xx xx xx 文件关联后缀名
	//keword:xxx
	m_userDefineKeyword = qs.value("keyword").toString().toUtf8();

	return m_userDefineKeyword.data();

}

QByteArray QsciLexer::getCommentLineSymbol()
{
    return m_commentSymbol;
}

void QsciLexer::setCommentLineSymbol(QByteArray comment)
{
    m_commentSymbol = comment;
}

QByteArray QsciLexer::getCommentStart()
{
    return m_commentStart;
}

QByteArray QsciLexer::getCommentEnd()
{
    return m_commentEnd;
}

void QsciLexer::setCommentStart(QByteArray commentStart)
{
    m_commentStart = commentStart;
}

void QsciLexer::setCommentEnd(QByteArray commentEnd)
{
    m_commentEnd = commentEnd;
}

void QsciLexer::setCurThemes(int themesId)
{
    m_themesId = themesId;
}
