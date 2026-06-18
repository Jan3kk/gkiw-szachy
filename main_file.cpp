/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define _USE_MATH_DEFINES

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <cmath>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "pieces.h"
#include "chess.h"

float aspectRatio = 1;

// Programy cieniujace
ShaderProgram *spLit;	// oswietlenie 2 zrodlami (bierki, plansza, stol, ziemia)
ShaderProgram *spSky;	// tekstura bez cieniowania (niebo)
ShaderProgram *spDepth; // przebieg glebii do mapy cieni

GLuint texSky;

// Tekstury materialow (kolor + normalna + szorstkosc)
GLuint texWoodColor = 0, texWoodNormal = 0, texWoodRough = 0;    // stol
GLuint texBoardColor = 0, texBoardNormal = 0, texBoardRough = 0; // szachownica
GLuint texWhiteColor = 0, texWhiteNormal = 0, texWhiteRough = 0; // biale bierki (marmur)
GLuint texBlackColor = 0, texBlackNormal = 0, texBlackRough = 0; // czarne bierki (marmur)

// Stan teksturowania biezacego obiektu (ustawiany przed rysowaniem)
int gUseTex = 0;
glm::vec2 gUvScale = glm::vec2(1.0f, 1.0f);
glm::vec2 gUvOffset = glm::vec2(0.0f, 0.0f);
GLuint gTexColor = 0, gTexNormal = 0, gTexRough = 0;

// Cienie (shadow mapping)
GLuint shadowFBO = 0, shadowTex = 0;
const int SHADOW_SIZE = 2048;
glm::mat4 lightSpaceMatrix; // P*V swiatla
bool shadowPass = false;	// czy trwa przebieg glebii (true) czy normalne rysowanie

// Macierze widoku i rzutowania (ustawiane co klatke)
glm::mat4 matV;
glm::mat4 matP;

// Pozycje dwoch zrodel swiatla (przestrzen swiata)
// Glowne swiatlo - ustawione w kierunku najjasniejszej "gwiazdy" w teksturze nieba
// (kierunek ~(-0.71, 0.70, -0.01), elewacja ~44 st.). Stad liczone sa cienie.
glm::vec4 light0 = glm::vec4(-35.7f, 38.0f, -0.3f, 1.0f);
// Slabe, chlodne doswietlenie z gory - drugie zrodlo (wymog >= 2 swiatel)
glm::vec4 light1 = glm::vec4(10.0f, 16.0f, 3.0f, 1.0f);

// Kolory swiatel i swiatla otoczenia - chlodny, nocny klimat pasujacy do skyboxa
glm::vec3 lightColor0 = glm::vec3(0.80f, 0.86f, 1.00f);	 // chlodne swiatlo "ksiezyca"
glm::vec3 lightColor1 = glm::vec3(0.18f, 0.22f, 0.34f);	 // slabe chlodne wypelnienie
glm::vec3 ambientColor = glm::vec3(0.12f, 0.14f, 0.20f); // ciemny, niebieskawy ambient
float matSpec = 0.3f; // sila odblaskow biezacego materialu (0 = matowy)

// Obrot stolu (i wszystkiego na nim) wokol pionowej osi - zeby cien padal ukosem do bokow stolu
glm::mat4 worldRot = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

// --- Kamera: swobodny lot (WASD + mysz) ---
glm::vec3 cameraPos = glm::vec3(0.0f, 9.0f, 16.0f);
float cameraYaw = -90.0f;
float cameraPitch = -30.0f;

bool keyW = false, keyS = false, keyA = false, keyD = false;
bool keyUp = false, keyDown = false; // spacja / lewy shift

double lastMouseX = 0.0, lastMouseY = 0.0;
bool firstMouse = true;

glm::vec3 cameraFront()
{
	float y = glm::radians(cameraYaw);
	float p = glm::radians(cameraPitch);
	return glm::normalize(glm::vec3(cosf(y) * cosf(p), sinf(p), sinf(y) * cosf(p)));
}

// skroty na macierze przeksztalcen
static glm::mat4 T(float x, float y, float z) { return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)); }
static glm::mat4 Sc(float x, float y, float z) { return glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z)); }
static glm::mat4 Ry(float a) { return glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f)); }
static glm::mat4 Rx(float a) { return glm::rotate(glm::mat4(1.0f), a, glm::vec3(1.0f, 0.0f, 0.0f)); }

// --- Geometria ---
Mesh mGround, mCube;
Mesh mPawn, mBishop, mRook, mQueen, mKing;
Mesh mKnightBase, mKnNeck, mKnHead, mKnSnout, mKnEar;
Mesh mMerlon, mSmallBall, mBall, mCrossV, mCrossH;

// Niebo (osobno - tekstura, bez normalnych)
std::vector<float> skyVerts;
std::vector<float> skyTex;

void buildGround(float s)
{
	glm::vec3 n(0.0f, 1.0f, 0.0f);
	pushVN(mGround, glm::vec3(-s, 0, -s), n);
	pushVN(mGround, glm::vec3(s, 0, -s), n);
	pushVN(mGround, glm::vec3(s, 0, s), n);
	pushVN(mGround, glm::vec3(-s, 0, -s), n);
	pushVN(mGround, glm::vec3(s, 0, s), n);
	pushVN(mGround, glm::vec3(-s, 0, s), n);
	mGround.count = (int)mGround.verts.size() / 3;
}

void buildSky(int stacks, int sectors)
{
	for (int i = 0; i < stacks; i++)
	{
		float phi1 = PI * (float)i / stacks;
		float phi2 = PI * (float)(i + 1) / stacks;
		for (int j = 0; j < sectors; j++)
		{
			float t1 = 2.0f * PI * (float)j / sectors;
			float t2 = 2.0f * PI * (float)(j + 1) / sectors;
			glm::vec3 a(sinf(phi1) * cosf(t1), cosf(phi1), sinf(phi1) * sinf(t1));
			glm::vec3 b(sinf(phi2) * cosf(t1), cosf(phi2), sinf(phi2) * sinf(t1));
			glm::vec3 c(sinf(phi2) * cosf(t2), cosf(phi2), sinf(phi2) * sinf(t2));
			glm::vec3 d(sinf(phi1) * cosf(t2), cosf(phi1), sinf(phi1) * sinf(t2));
			float u1 = (float)j / sectors, u2 = (float)(j + 1) / sectors;
			float v1 = (float)i / stacks, v2 = (float)(i + 1) / stacks;
			skyVerts.push_back(a.x);
			skyVerts.push_back(a.y);
			skyVerts.push_back(a.z);
			skyTex.push_back(u1);
			skyTex.push_back(v1);
			skyVerts.push_back(b.x);
			skyVerts.push_back(b.y);
			skyVerts.push_back(b.z);
			skyTex.push_back(u1);
			skyTex.push_back(v2);
			skyVerts.push_back(c.x);
			skyVerts.push_back(c.y);
			skyVerts.push_back(c.z);
			skyTex.push_back(u2);
			skyTex.push_back(v2);
			skyVerts.push_back(a.x);
			skyVerts.push_back(a.y);
			skyVerts.push_back(a.z);
			skyTex.push_back(u1);
			skyTex.push_back(v1);
			skyVerts.push_back(c.x);
			skyVerts.push_back(c.y);
			skyVerts.push_back(c.z);
			skyTex.push_back(u2);
			skyTex.push_back(v2);
			skyVerts.push_back(d.x);
			skyVerts.push_back(d.y);
			skyVerts.push_back(d.z);
			skyTex.push_back(u2);
			skyTex.push_back(v1);
		}
	}
}

// --- Stan partii i plansza ---
const float SURFACE_Y = 3.5f;	// gorna plaszczyzna pol - tu stoja bierki
const float TABLE_TOP_Y = 3.2f; // gorna plaszczyzna blatu (rzad zbitych figur)

static float wX(int f) { return f - 3.5f; }
static float wZ(int r) { return 3.5f - r; }

struct Inst
{
	int type;
	int color;
	glm::vec3 pos;
	bool captured = false;
};

std::vector<Inst> insts;
int occ[8][8];

std::vector<Move> game;
int currentMove = 0;
float moveTimer = 0.0f;
bool playing = true;
const float MOVE_DUR = 0.9f;
const float MOVE_PAUSE = 0.35f;
int capWhite = 0, capBlack = 0;

struct Anim
{
	int prim = -1, pf = 0, pr = 0, ptf = 0, ptr = 0;
	glm::vec3 pFrom, pTo;
	bool arc = false;
	int sec = -1, sf = 0, sr = 0, stf = 0, str = 0;
	glm::vec3 sFrom, sTo;
	int cap = -1, capf = 0, capr = 0;
	glm::vec3 cFrom, cTo;
	int promo = PT_EMPTY;
};
Anim cur;
bool curActive = false;

void setupBoard()
{
	PType t[8][8];
	PColor c[8][8];
	startingPosition(t, c);
	insts.clear();
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
			occ[f][r] = -1;
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
			if (t[f][r] != PT_EMPTY)
			{
				Inst it;
				it.type = t[f][r];
				it.color = c[f][r];
				it.pos = glm::vec3(wX(f), SURFACE_Y, wZ(r));
				occ[f][r] = (int)insts.size();
				insts.push_back(it);
			}
}

// Kolejne wolne miejsce w rzedzie zbitych figur danego koloru
glm::vec3 graveyardSlot(int color)
{
	int idx = (color == PC_WHITE) ? capWhite++ : capBlack++;
	float side = (color == PC_WHITE) ? -5.3f : 5.3f;
	int row = idx / 8, k = idx % 8;
	float x = side + ((color == PC_WHITE) ? -row * 0.9f : row * 0.9f);
	float z = -3.15f + k * 0.9f;
	return glm::vec3(x, TABLE_TOP_Y, z);
}

// Przygotowanie animacji ruchu nr idx (na podstawie aktualnego stanu planszy)
void startMove(int idx)
{
	cur = Anim();
	int color = (idx % 2 == 0) ? PC_WHITE : PC_BLACK;
	Move m = game[idx];

	if (m.castleK || m.castleQ)
	{
		int rank = (color == PC_WHITE) ? 0 : 7;
		int ktf = m.castleK ? 6 : 2;
		int rf = m.castleK ? 7 : 0;
		int rtf = m.castleK ? 5 : 3;
		cur.prim = occ[4][rank];
		cur.pf = 4;
		cur.pr = rank;
		cur.ptf = ktf;
		cur.ptr = rank;
		cur.sec = occ[rf][rank];
		cur.sf = rf;
		cur.sr = rank;
		cur.stf = rtf;
		cur.str = rank;
		if (cur.prim >= 0)
			cur.pFrom = insts[cur.prim].pos;
		cur.pTo = glm::vec3(wX(ktf), SURFACE_Y, wZ(rank));
		if (cur.sec >= 0)
			cur.sFrom = insts[cur.sec].pos;
		cur.sTo = glm::vec3(wX(rtf), SURFACE_Y, wZ(rank));
		curActive = true;
		return;
	}

	cur.prim = occ[m.ff][m.fr];
	cur.pf = m.ff;
	cur.pr = m.fr;
	cur.ptf = m.tf;
	cur.ptr = m.tr;
	cur.promo = m.promo;
	if (cur.prim >= 0)
	{
		cur.pFrom = insts[cur.prim].pos;
		cur.arc = (insts[cur.prim].type == PT_KNIGHT);
	}
	else
		cur.pFrom = glm::vec3(wX(m.ff), SURFACE_Y, wZ(m.fr));
	cur.pTo = glm::vec3(wX(m.tf), SURFACE_Y, wZ(m.tr));

	// Bicie zwykle lub w przelocie (en passant)
	if (occ[m.tf][m.tr] >= 0)
	{
		cur.cap = occ[m.tf][m.tr];
		cur.capf = m.tf;
		cur.capr = m.tr;
	}
	else if (cur.prim >= 0 && insts[cur.prim].type == PT_PAWN && m.ff != m.tf && occ[m.tf][m.fr] >= 0)
	{
		cur.cap = occ[m.tf][m.fr];
		cur.capf = m.tf;
		cur.capr = m.fr;
	}
	if (cur.cap >= 0)
	{
		cur.cFrom = insts[cur.cap].pos;
		cur.cTo = graveyardSlot(insts[cur.cap].color);
	}
	curActive = true;
}

// Zatwierdzenie ruchu - aktualizacja logicznego stanu planszy
void commitMove()
{
	if (!curActive)
		return;
	if (cur.cap >= 0)
	{
		insts[cur.cap].captured = true;
		insts[cur.cap].pos = cur.cTo;
		occ[cur.capf][cur.capr] = -1;
	}
	if (cur.prim >= 0)
	{
		occ[cur.pf][cur.pr] = -1;
		insts[cur.prim].pos = cur.pTo;
		if (cur.promo != PT_EMPTY)
			insts[cur.prim].type = cur.promo;
		occ[cur.ptf][cur.ptr] = cur.prim;
	}
	if (cur.sec >= 0)
	{
		occ[cur.sf][cur.sr] = -1;
		insts[cur.sec].pos = cur.sTo;
		occ[cur.stf][cur.str] = cur.sec;
	}
	curActive = false;
}

// Pozycja zbijanej bierki: w gore -> nad rzad zbitych -> w dol na blat
glm::vec3 capturedAnimPos(float t)
{
	float lift = 2.6f;
	glm::vec3 a = cur.cFrom, b = cur.cTo;
	if (t < 0.34f)
	{
		float u = t / 0.34f;
		return glm::vec3(a.x, a.y + lift * u, a.z);
	}
	else if (t < 0.68f)
	{
		float u = (t - 0.34f) / 0.34f;
		return glm::vec3(glm::mix(a.x, b.x, u), a.y + lift, glm::mix(a.z, b.z, u));
	}
	float u = (t - 0.68f) / 0.32f;
	return glm::vec3(b.x, glm::mix(a.y + lift, b.y, u), b.z);
}

// Pozycja, w ktorej nalezy narysowac bierke i (z uwzglednieniem animacji)
glm::vec3 renderPos(int i)
{
	if (curActive)
	{
		float t = moveTimer / MOVE_DUR;
		if (t > 1.0f)
			t = 1.0f;
		if (t < 0.0f)
			t = 0.0f;
		float ts = t * t * (3.0f - 2.0f * t); // smoothstep
		if (i == cur.prim)
		{
			glm::vec3 p = glm::mix(cur.pFrom, cur.pTo, ts);
			if (cur.arc)
				p.y += 2.3f * sinf(PI * t); // skoczek przeskakuje
			return p;
		}
		if (i == cur.sec)
			return glm::mix(cur.sFrom, cur.sTo, ts);
		if (i == cur.cap)
			return capturedAnimPos(t);
	}
	return insts[i].pos;
}

void error_callback(int error, const char *description) { fputs(description, stderr); }

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	bool pressed = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_W)
		keyW = pressed;
	if (key == GLFW_KEY_S)
		keyS = pressed;
	if (key == GLFW_KEY_A)
		keyA = pressed;
	if (key == GLFW_KEY_D)
		keyD = pressed;
	if (key == GLFW_KEY_SPACE)
		keyUp = pressed;
	if (key == GLFW_KEY_LEFT_SHIFT)
		keyDown = pressed;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void cursorCallback(GLFWwindow *window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
	}
	float dx = (float)(xpos - lastMouseX);
	float dy = (float)(lastMouseY - ypos);
	lastMouseX = xpos;
	lastMouseY = ypos;
	const float sens = 0.12f;
	cameraYaw += dx * sens;
	cameraPitch += dy * sens;
	if (cameraPitch > 89.0f)
		cameraPitch = 89.0f;
	if (cameraPitch < -89.0f)
		cameraPitch = -89.0f;
}

void windowResizeCallback(GLFWwindow *window, int width, int height)
{
	if (height == 0)
		return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

GLuint readTexture(const char *filename, bool clampVertical = false)
{
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char *)image.data());
	glGenerateMipmap(GL_TEXTURE_2D); // mip-mapy - mniej migotania w oddali
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);		 // poziomo: panorama zawija sie bez szwu
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampVertical ? GL_CLAMP_TO_EDGE : GL_REPEAT); // pionowo: bez artefaktu na biegunie
	return tex;
}

// --- Inicjalizacja ---
void initOpenGLProgram(GLFWwindow *window)
{
	glClearColor(0.03f, 0.03f, 0.07f, 1.0f); // ciemne tlo pasujace do nocnego nieba
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	spLit = new ShaderProgram("v_lit.glsl", NULL, "f_lit.glsl");
	spSky = new ShaderProgram("v_tex.glsl", NULL, "f_tex.glsl");
	spDepth = new ShaderProgram("v_depth.glsl", NULL, "f_depth.glsl");
	texSky = readTexture("sky.png", true); // niebo - pionowo CLAMP
	texWoodColor = readTexture("wood_color.png");
	texWoodNormal = readTexture("wood_normal.png");
	texWoodRough = readTexture("wood_rough.png");
	gTexColor = texWoodColor; // domyslne tekstury zwiazane przy rysowaniu (uzywane gdy useTex=1)
	gTexNormal = texWoodNormal;
	gTexRough = texWoodRough;
	texBoardColor = readTexture("board_color.png");
	texBoardNormal = readTexture("board_normal.png");
	texBoardRough = readTexture("board_rough.png");
	texWhiteColor = readTexture("marbleW_color.png");
	texWhiteNormal = readTexture("marbleW_normal.png");
	texWhiteRough = readTexture("marbleW_rough.png");
	texBlackColor = readTexture("marbleB_color.png");
	texBlackNormal = readTexture("marbleB_normal.png");
	texBlackRough = readTexture("marbleB_rough.png");

	// Bufor glebii na mape cieni (FBO + tekstura glebii)
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_SIZE, SHADOW_SIZE, 0,
				 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float border[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // poza mapa cieni = brak cienia
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE); // brak bufora koloru - tylko glebia
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	buildGround(450.0f);
	buildSky(24, 48);
	mCube = makeBox(1.0f, 1.0f, 1.0f);
	mPawn = makePawn();
	mBishop = makeBishop();
	mRook = makeRook();
	mQueen = makeQueen();
	mKing = makeKing();
	mKnightBase = makeKnightBase();
	mKnNeck = makeBox(0.24f, 0.55f, 0.26f);
	mKnHead = makeBox(0.22f, 0.26f, 0.44f);
	mKnSnout = makeBox(0.18f, 0.18f, 0.22f);
	mKnEar = makeBox(0.06f, 0.16f, 0.06f);
	mMerlon = makeBox(0.13f, 0.18f, 0.13f);
	mSmallBall = makeSphere(0.06f, 10, 12);
	mBall = makeSphere(0.08f, 12, 16);
	mCrossV = makeBox(0.07f, 0.34f, 0.07f);
	mCrossH = makeBox(0.22f, 0.07f, 0.07f);

	setupBoard();

	std::string err;
	if (!parseGame("games/szewczyk.txt", game, err))
		fprintf(stderr, "Blad wczytywania partii: %s\n", err.c_str());
	if (!game.empty())
		startMove(0);
	else
		playing = false;
}

void freeOpenGLProgram(GLFWwindow *window)
{
	delete spLit;
	delete spSky;
	delete spDepth;
}

// --- Rysowanie ---
void drawLit(const Mesh &m, const glm::mat4 &M, const glm::vec4 &color)
{
	// Przebieg glebii (mapa cieni): rysujemy tylko pozycje z punktu widzenia swiatla
	if (shadowPass)
	{
		spDepth->use();
		glm::mat4 mvp = lightSpaceMatrix * M;
		glUniformMatrix4fv(spDepth->u("MVP"), 1, false, glm::value_ptr(mvp));
		glEnableVertexAttribArray(spDepth->a("vertex"));
		glVertexAttribPointer(spDepth->a("vertex"), 3, GL_FLOAT, false, 0, m.verts.data());
		glDrawArrays(GL_TRIANGLES, 0, m.count);
		glDisableVertexAttribArray(spDepth->a("vertex"));
		return;
	}

	spLit->use();
	glUniformMatrix4fv(spLit->u("P"), 1, false, glm::value_ptr(matP));
	glUniformMatrix4fv(spLit->u("V"), 1, false, glm::value_ptr(matV));
	glUniformMatrix4fv(spLit->u("M"), 1, false, glm::value_ptr(M));
	glUniformMatrix4fv(spLit->u("lightSpaceMatrix"), 1, false, glm::value_ptr(lightSpaceMatrix));
	glUniform4fv(spLit->u("color"), 1, glm::value_ptr(color));
	glm::vec3 ld0 = glm::normalize(glm::vec3(light0) - glm::vec3(0.0f, 3.0f, 0.0f));
	glm::vec3 ld1 = glm::normalize(glm::vec3(light1) - glm::vec3(0.0f, 3.0f, 0.0f));
	glUniform3fv(spLit->u("lightDir0"), 1, glm::value_ptr(ld0));
	glUniform3fv(spLit->u("lightDir1"), 1, glm::value_ptr(ld1));
	glUniform1f(spLit->u("specStrength"), matSpec);
	glUniform3fv(spLit->u("lc0"), 1, glm::value_ptr(lightColor0));
	glUniform3fv(spLit->u("lc1"), 1, glm::value_ptr(lightColor1));
	glUniform3fv(spLit->u("ambientColor"), 1, glm::value_ptr(ambientColor));
	glUniform1i(spLit->u("shadowMap"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	// Tekstury materialu (color/normal/roughness) - uzywane tylko gdy useTex==1
	bool textured = (gUseTex == 1 && !m.tex.empty());
	glUniform1i(spLit->u("useTex"), textured ? 1 : 0);
	glUniform2fv(spLit->u("uvScale"), 1, glm::value_ptr(gUvScale));
	glUniform2fv(spLit->u("uvOffset"), 1, glm::value_ptr(gUvOffset));
	glUniform1i(spLit->u("diffuseMap"), 0);
	glUniform1i(spLit->u("normalMap"), 2);
	glUniform1i(spLit->u("roughMap"), 3);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexColor);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gTexNormal);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gTexRough);

	glEnableVertexAttribArray(spLit->a("vertex"));
	glVertexAttribPointer(spLit->a("vertex"), 3, GL_FLOAT, false, 0, m.verts.data());
	glEnableVertexAttribArray(spLit->a("normal"));
	glVertexAttribPointer(spLit->a("normal"), 3, GL_FLOAT, false, 0, m.norms.data());
	if (textured)
	{
		glEnableVertexAttribArray(spLit->a("texCoord0"));
		glVertexAttribPointer(spLit->a("texCoord0"), 2, GL_FLOAT, false, 0, m.tex.data());
		glEnableVertexAttribArray(spLit->a("tangent"));
		glVertexAttribPointer(spLit->a("tangent"), 3, GL_FLOAT, false, 0, m.tans.data());
	}

	glDrawArrays(GL_TRIANGLES, 0, m.count);

	glDisableVertexAttribArray(spLit->a("vertex"));
	glDisableVertexAttribArray(spLit->a("normal"));
	if (textured)
	{
		glDisableVertexAttribArray(spLit->a("texCoord0"));
		glDisableVertexAttribArray(spLit->a("tangent"));
	}
}

void drawSky()
{
	glm::mat4 M = T(cameraPos.x, cameraPos.y, cameraPos.z) * Sc(200.0f, 200.0f, 200.0f);
	spSky->use();
	glUniformMatrix4fv(spSky->u("P"), 1, false, glm::value_ptr(matP));
	glUniformMatrix4fv(spSky->u("V"), 1, false, glm::value_ptr(matV));
	glUniformMatrix4fv(spSky->u("M"), 1, false, glm::value_ptr(M));
	glUniform1i(spSky->u("textureMap0"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texSky);
	glEnableVertexAttribArray(spSky->a("vertex"));
	glVertexAttribPointer(spSky->a("vertex"), 3, GL_FLOAT, false, 0, skyVerts.data());
	glEnableVertexAttribArray(spSky->a("texCoord0"));
	glVertexAttribPointer(spSky->a("texCoord0"), 2, GL_FLOAT, false, 0, skyTex.data());
	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLES, 0, (int)skyVerts.size() / 3);
	glDepthMask(GL_TRUE);
	glDisableVertexAttribArray(spSky->a("vertex"));
	glDisableVertexAttribArray(spSky->a("texCoord0"));
}

void drawTable()
{
	// Stol oteksturowany drewnem (kolor + mapa normalnych + szorstkosc)
	matSpec = 0.5f; // bazowa sila odblasku (modulowana mapa roughness)
	gUseTex = 1;
	gTexColor = texWoodColor;
	gTexNormal = texWoodNormal;
	gTexRough = texWoodRough;
	glm::vec4 tint(1.0f, 1.0f, 1.0f, 1.0f); // bialy odcien = tekstura bez przebarwienia

	gUvScale = glm::vec2(3.0f, 3.0f); // blat
	drawLit(mCube, worldRot * T(0, 3.0f, 0) * Sc(12.0f, 0.4f, 12.0f), tint);

	gUvScale = glm::vec2(1.0f, 3.0f); // nogi - sloje wzdluz
	float legH = 2.8f, off = 5.2f;
	for (int sx = -1; sx <= 1; sx += 2)
		for (int sz = -1; sz <= 1; sz += 2)
			drawLit(mCube, worldRot * T(sx * off, legH * 0.5f, sz * off) * Sc(0.5f, legH, 0.5f), tint);

	gUseTex = 0;
	gUvScale = glm::vec2(1.0f, 1.0f);
}

void drawBoard()
{
	// Szachownica oteksturowana drewnem (Wood060); wzor pol = odcien (jasne/ciemne)
	matSpec = 0.85f; // szachownica - mocniejszy polysk
	gUseTex = 1;
	gTexColor = texBoardColor;
	gTexNormal = texBoardNormal;
	gTexRough = texBoardRough;
	float top = 3.2f;

	// Rama planszy - ciemniejsze drewno
	gUvScale = glm::vec2(3.0f, 3.0f);
	gUvOffset = glm::vec2(0.0f, 0.0f);
	drawLit(mCube, worldRot * T(0, top + 0.1f, 0) * Sc(9.0f, 0.2f, 9.0f), glm::vec4(0.45f, 0.30f, 0.17f, 1.0f));

	// 64 pola - to samo drewno, rozny odcien i inny wycinek slojow na kazdym polu
	glm::vec4 light(1.00f, 0.86f, 0.66f, 1.0f); // jasne pola
	glm::vec4 dark(0.42f, 0.27f, 0.15f, 1.0f);  // ciemne pola
	float sq = top + 0.2f;
	gUvScale = glm::vec2(1.0f, 1.0f);
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
		{
			glm::vec4 col = ((f + r) % 2 == 0) ? dark : light;
			gUvOffset = glm::vec2(f * 0.137f + r * 0.219f, r * 0.137f + f * 0.219f); // rozny wycinek slojow
			drawLit(mCube, worldRot * T(wX(f), sq + 0.05f, wZ(r)) * Sc(1.0f, 0.1f, 1.0f), col);
		}

	gUseTex = 0;
	gUvScale = glm::vec2(1.0f, 1.0f);
	gUvOffset = glm::vec2(0.0f, 0.0f);
}

void drawKnight(const glm::mat4 &base, int color, const glm::vec4 &col)
{
	glm::mat4 f = base * Ry(color == PC_WHITE ? 0.0f : PI);
	drawLit(mKnightBase, f, col);
	drawLit(mKnNeck, f * T(0, 0.45f, -0.02f) * Rx(-0.30f), col);
	drawLit(mKnHead, f * T(0, 0.78f, 0.10f) * Rx(-0.10f), col);
	drawLit(mKnSnout, f * T(0, 0.80f, 0.34f), col);
	drawLit(mKnEar, f * T(-0.07f, 0.98f, -0.02f), col);
	drawLit(mKnEar, f * T(0.07f, 0.98f, -0.02f), col);
}

void drawPiece(const Inst &it, const glm::vec3 &pos, int seed)
{
	// Bierki z marmuru: biale = Marble012, czarne = Marble016
	matSpec = 0.7f; // marmur - wyrazny polysk (modulowany roughness)
	gUseTex = 1;
	if (it.color == PC_WHITE)
	{
		gTexColor = texWhiteColor;
		gTexNormal = texWhiteNormal;
		gTexRough = texWhiteRough;
	}
	else
	{
		gTexColor = texBlackColor;
		gTexNormal = texBlackNormal;
		gTexRough = texBlackRough;
	}
	gUvScale = glm::vec2(1.0f, 1.0f);
	gUvOffset = glm::vec2(seed * 0.37f, seed * 0.61f); // inny wycinek marmuru na kazdej bierce
	glm::vec4 col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // bialy odcien = marmur bez przebarwienia
	glm::mat4 base = worldRot * T(pos.x, pos.y, pos.z);
	switch (it.type)
	{
	case PT_PAWN:
		drawLit(mPawn, base, col);
		break;
	case PT_BISHOP:
		drawLit(mBishop, base, col);
		drawLit(mBall, base * T(0, 1.24f, 0), col);
		break;
	case PT_ROOK:
		drawLit(mRook, base, col);
		for (int k = 0; k < 8; k++)
			drawLit(mMerlon, base * Ry(k * PI / 4.0f) * T(0.24f, 0.83f, 0.0f), col);
		break;
	case PT_QUEEN:
		drawLit(mQueen, base, col);
		for (int k = 0; k < 8; k++)
			drawLit(mSmallBall, base * Ry(k * PI / 4.0f) * T(0.25f, 1.40f, 0.0f), col);
		drawLit(mSmallBall, base * T(0, 1.52f, 0), col);
		break;
	case PT_KING:
		drawLit(mKing, base, col);
		drawLit(mCrossV, base * T(0, 1.72f, 0), col);
		drawLit(mCrossH, base * T(0, 1.70f, 0), col);
		break;
	case PT_KNIGHT:
		drawKnight(base, it.color, col);
		break;
	}
}

// Cala geometria sceny - uzywana w obu przebiegach (glebii i koloru)
void renderObjects()
{
	gUseTex = 0; // ziemia bez tekstury
	matSpec = 0.0f;															 // matowa ziemia - bez wedrujacych odblaskow
	drawLit(mGround, glm::mat4(1.0f), glm::vec4(0.10f, 0.18f, 0.19f, 1.0f)); // stonowana, niebieskawa "nocna" trawa
	drawTable();
	drawBoard();
	for (int i = 0; i < (int)insts.size(); i++)
		drawPiece(insts[i], renderPos(i), i);
}

void drawScene(GLFWwindow *window)
{
	// Macierz swiatla: rzut ortogonalny od swiatla glownego na obszar planszy/stolu
	glm::vec3 lcenter = glm::vec3(0.0f, 3.0f, 0.0f);
	glm::vec3 ldir = glm::normalize(glm::vec3(light0) - lcenter);
	glm::vec3 lup = (fabs(ldir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	float ldist = glm::length(glm::vec3(light0) - lcenter);
	float lnear = ldist - 25.0f;
	if (lnear < 1.0f)
		lnear = 1.0f;
	glm::mat4 lightView = glm::lookAt(glm::vec3(light0), lcenter, lup);
	glm::mat4 lightProj = glm::ortho(-13.0f, 13.0f, -13.0f, 13.0f, lnear, ldist + 30.0f);
	lightSpaceMatrix = lightProj * lightView;

	// PRZEBIEG 1: glebia z punktu widzenia swiatla -> mapa cieni
	shadowPass = true;
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f); // odsuniecie glebii - mniej artefaktow "shadow acne"
	renderObjects();
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// PRZEBIEG 2: normalne rysowanie sceny z cieniem
	shadowPass = false;
	int fbw, fbh;
	glfwGetFramebufferSize(window, &fbw, &fbh);
	glViewport(0, 0, fbw, fbh);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec3 front = cameraFront();
	matV = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
	matP = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);

	drawSky();
	renderObjects();

	glfwSwapBuffers(window);
}

// Posuwa odtwarzanie partii do przodu
void updateGame(float dt)
{
	if (!playing || currentMove >= (int)game.size())
		return;
	moveTimer += dt;
	if (moveTimer >= MOVE_DUR + MOVE_PAUSE)
	{
		commitMove();
		currentMove++;
		moveTimer = 0.0f;
		if (currentMove < (int)game.size())
			startMove(currentMove);
		else
			playing = false;
	}
}

int main(void)
{
	GLFWwindow *window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1000, 700, "Szachy 3D - odtwarzacz partii", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	glfwSetTime(0);
	while (!glfwWindowShouldClose(window))
	{
		float dt = (float)glfwGetTime();
		glfwSetTime(0);

		glm::vec3 front = cameraFront();
		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		float speed = 14.0f * dt;
		if (keyW)
			cameraPos += front * speed;
		if (keyS)
			cameraPos -= front * speed;
		if (keyA)
			cameraPos -= right * speed;
		if (keyD)
			cameraPos += right * speed;
		if (keyUp)
			cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * speed;
		if (keyDown)
			cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * speed;

		updateGame(dt);
		drawScene(window);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
