#include "EntityList.h"



static char *ENTITY_NAMES[] = { 
  "Aacute",
  "aacute",
  "Acirc",
  "acirc",
  "acute",
  "AElig",
  "aelig",
  "Agrave",
  "agrave",
  "amp",
  "apos",
  "Aring",
  "aring",
  "Atilde",
  "atilde",
  "Auml",
  "auml",
  "brvbar",
  "Ccedil",
  "ccedil",
  "cedil",
  "cent",
  "copy",
  "curren",
  "deg",
  "divide",
  "Eacute",
  "eacute",
  "Ecirc",
  "ecirc",
  "Egrave",
  "egrave",
  "ETH",
  "eth",
  "Euml",
  "euml",
  "frac12",
  "frac14",
  "frac34",
  "gt",
  "Iacute",
  "iacute",
  "Icirc",
  "icirc",
  "iexcl",
  "Igrave",
  "igrave",
  "iquest",
  "Iuml",
  "iuml",
  "laquo",
  "lt",
  "macr",
  "micro",
  "middot",
  "nbsp",
  "not",
  "Ntilde",
  "ntilde",
  "Oacute",
  "oacute",
  "Ocirc",
  "ocirc",
  "Ograve",
  "ograve",
  "ordf",
  "ordm",
  "Oslash",
  "oslash",
  "Otilde",
  "otilde",
  "Ouml",
  "ouml",
  "para",
  "plusmn",
  "pound",
  "quot",
  "raquo",
  "reg",
  "sect",
  "shy",
  "sup1",
  "sup2",
  "sup3",
  "szlig",
  "THORN",
  "thorn",
  "times",
  "Uacute",
  "uacute",
  "Ucirc",
  "ucirc",
  "Ugrave",
  "ugrave",
  "uml",
  "Uuml",
  "uuml",
  "Yacute",
  "yacute",
  "yen",
};

static int ENTITY_DEF[] = {
  193,
  225,
  194,
  226,
  180,
  198,
  230,
  192,
  224,
  38,
  39,
  197,
  229,
  195,
  227,
  196,
  228,
  166,
  199,
  231,
  184,
  162,
  169,
  164,
  176,
  247,
  201,
  233,
  202,
  234,
  200,
  232,
  208,
  240,
  203,
  235,
  189,
  188,
  190,
  62,
  205,
  237,
  206,
  238,
  161,
  204,
  236,
  191,
  207,
  239,
  171,
  60,
  175,
  181,
  183,
  160,
  172,
  209,
  241,
  211,
  243,
  212,
  244,
  210,
  242,
  170,
  186,
  216,
  248,
  213,
  245,
  214,
  246,
  182,
  177,
  163,
  34,
  187,
  174,
  167,
  173,
  185,
  178,
  179,
  223,
  222,
  254,
  215,
  218,
  250,
  219,
  251,
  217,
  249,
  168,
  220,
  252,
  221,
  253,
  165,
};
#define NUM_MXP_ENTITIES 100

bool EntityList::m_init = false;
QHash<QString,QString> EntityList::m_dec;
QHash<QString,QString> EntityList::m_enc;
	
void EntityList::init()
{
	if(m_init)
		return;
		
	char str[2];
	str[1] = 0;
	
	QString chr;
	QString name;
	QString ent;
	
	for (int i = 0; i < NUM_MXP_ENTITIES; i++)
	{
		str[0] = ENTITY_DEF[i];
		chr  = QString::fromLatin1(str);
		
		name = QString::fromLatin1(ENTITY_NAMES[i]);
		ent  = QString("&%1;").arg(name);
		
		m_dec[ent] = chr;
		m_enc[chr] = ent;
	}
	
	m_init = true;
}

QString EntityList::decodeEntities(QString str)
{
	init();
	
	foreach(QString ent, m_dec.values())
		str = str.replace(ent, m_dec[ent]);
	
	return str;
}

QString EntityList::encodeEntities(QString str)
{
	init();
	
	foreach(QString ent, m_enc.values())
		str = str.replace(ent, m_enc[ent]);
	
	return str;
}

