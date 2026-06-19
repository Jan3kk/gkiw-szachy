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

ShaderProgram *spLit;
ShaderProgram *spSky;
ShaderProgram *spDepth;

GLuint texSky;

GLuint texWoodColor = 0, texWoodNormal = 0, texWoodRough = 0;
GLuint texBoardColor = 0, texBoardNormal = 0, texBoardRough = 0;
GLuint texWhiteColor = 0, texWhiteNormal = 0, texWhiteRough = 0;
GLuint texBlackColor = 0, texBlackNormal = 0, texBlackRough = 0;

int materialTextured = 0;
glm::vec2 materialUvScale = glm::vec2(1.0f, 1.0f);
glm::vec2 materialUvOffset = glm::vec2(0.0f, 0.0f);
GLuint materialColorMap = 0, materialNormalMap = 0, materialRoughMap = 0;

GLuint shadowFBO = 0, shadowTex = 0;
const int SHADOW_SIZE = 2048;
glm::mat4 lightSpaceMatrix;
bool shadowPass = false;

glm::mat4 matV;
glm::mat4 matP;

glm::vec4 mainLightPos = glm::vec4(-35.7f, 38.0f, -0.3f, 1.0f);
glm::vec4 fillLightPos = glm::vec4(10.0f, 16.0f, 3.0f, 1.0f);

glm::vec3 mainLightColor = glm::vec3(0.80f, 0.86f, 1.00f);
glm::vec3 fillLightColor = glm::vec3(0.18f, 0.22f, 0.34f);
glm::vec3 ambientColor = glm::vec3(0.12f, 0.14f, 0.20f);
float materialSpecular = 0.3f;

glm::mat4 worldRot = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

glm::vec3 cameraPos = glm::vec3(8.0f, 9.0f, 13.86f);
float cameraYaw = -120.0f;
float cameraPitch = -30.0f;

bool keyW = false, keyS = false, keyA = false, keyD = false;
bool keyUp = false, keyDown = false;
bool started = false;

double lastMouseX = 0.0, lastMouseY = 0.0;
bool firstMouse = true;

glm::vec3 cameraFront()
{
	float y = glm::radians(cameraYaw);
	float p = glm::radians(cameraPitch);
	return glm::normalize(glm::vec3(cosf(y) * cosf(p), sinf(p), sinf(y) * cosf(p)));
}

static glm::mat4 translate(float x, float y, float z) { return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)); }
static glm::mat4 scale(float x, float y, float z) { return glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z)); }
static glm::mat4 rotateY(float angle) { return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)); }
static glm::mat4 rotateX(float angle) { return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f)); }

Mesh mGround, mCube;
Mesh mPawn, mBishop, mRook, mQueen, mKing;
Mesh mKnightBase, mKnNeck, mKnHead, mKnSnout, mKnEar;
Mesh mMerlon, mSmallBall, mBall, mCrossV, mCrossH;

std::vector<float> skyVerts;
std::vector<float> skyTex;

void buildGround(float size)
{
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	addVertex(mGround, glm::vec3(-size, 0, -size), up);
	addVertex(mGround, glm::vec3(size, 0, -size), up);
	addVertex(mGround, glm::vec3(size, 0, size), up);
	addVertex(mGround, glm::vec3(-size, 0, -size), up);
	addVertex(mGround, glm::vec3(size, 0, size), up);
	addVertex(mGround, glm::vec3(-size, 0, size), up);
	mGround.vertexCount = (int)mGround.positions.size() / 3;
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

const float SURFACE_Y = 3.5f;
const float TABLE_TOP_Y = 3.2f;

static float worldX(int file) { return file - 3.5f; }
static float worldZ(int rank) { return 3.5f - rank; }

struct PieceInstance
{
	int type;
	int color;
	glm::vec3 pos;
	bool captured = false;
};

std::vector<PieceInstance> pieces;
int occupancy[8][8];

std::vector<Move> game;
int currentMove = 0;
float moveTimer = 0.0f;
bool playing = true;
const float MOVE_DURATION = 0.9f;
const float MOVE_PAUSE = 0.35f;
int capturedWhiteCount = 0, capturedBlackCount = 0;

struct MoveAnimation
{
	int mover = -1, fromFile = 0, fromRank = 0, toFile = 0, toRank = 0;
	glm::vec3 moverFrom, moverTo;
	bool jumpArc = false;
	int rook = -1, rookFromFile = 0, rookFromRank = 0, rookToFile = 0, rookToRank = 0;
	glm::vec3 rookFrom, rookTo;
	int capturedPiece = -1, capturedFile = 0, capturedRank = 0;
	glm::vec3 capturedFrom, capturedTo;
	int promotion = PIECE_NONE;
};
MoveAnimation currentAnim;
bool animActive = false;

void setupBoard()
{
	PieceType pieceTypes[8][8];
	PieceColor pieceColors[8][8];
	startingPosition(pieceTypes, pieceColors);
	pieces.clear();
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
			occupancy[f][r] = -1;
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
			if (pieceTypes[f][r] != PIECE_NONE)
			{
				PieceInstance piece;
				piece.type = pieceTypes[f][r];
				piece.color = pieceColors[f][r];
				piece.pos = glm::vec3(worldX(f), SURFACE_Y, worldZ(r));
				occupancy[f][r] = (int)pieces.size();
				pieces.push_back(piece);
			}
}

glm::vec3 graveyardSlot(int color)
{
	int slotIndex = (color == COLOR_WHITE) ? capturedWhiteCount++ : capturedBlackCount++;
	float sideX = (color == COLOR_WHITE) ? -5.3f : 5.3f;
	int row = slotIndex / 8, column = slotIndex % 8;
	float x = sideX + ((color == COLOR_WHITE) ? -row * 0.9f : row * 0.9f);
	float z = -3.15f + column * 0.9f;
	return glm::vec3(x, TABLE_TOP_Y, z);
}

void startMove(int moveIndex)
{
	currentAnim = MoveAnimation();
	int color = (moveIndex % 2 == 0) ? COLOR_WHITE : COLOR_BLACK;
	Move move = game[moveIndex];

	if (move.castleKingside || move.castleQueenside)
	{
		int rank = (color == COLOR_WHITE) ? 0 : 7;
		int kingToFile = move.castleKingside ? 6 : 2;
		int rookSrcFile = move.castleKingside ? 7 : 0;
		int rookDstFile = move.castleKingside ? 5 : 3;
		currentAnim.mover = occupancy[4][rank];
		currentAnim.fromFile = 4;
		currentAnim.fromRank = rank;
		currentAnim.toFile = kingToFile;
		currentAnim.toRank = rank;
		currentAnim.rook = occupancy[rookSrcFile][rank];
		currentAnim.rookFromFile = rookSrcFile;
		currentAnim.rookFromRank = rank;
		currentAnim.rookToFile = rookDstFile;
		currentAnim.rookToRank = rank;
		if (currentAnim.mover >= 0)
			currentAnim.moverFrom = pieces[currentAnim.mover].pos;
		currentAnim.moverTo = glm::vec3(worldX(kingToFile), SURFACE_Y, worldZ(rank));
		if (currentAnim.rook >= 0)
			currentAnim.rookFrom = pieces[currentAnim.rook].pos;
		currentAnim.rookTo = glm::vec3(worldX(rookDstFile), SURFACE_Y, worldZ(rank));
		animActive = true;
		return;
	}

	currentAnim.mover = occupancy[move.fromFile][move.fromRank];
	currentAnim.fromFile = move.fromFile;
	currentAnim.fromRank = move.fromRank;
	currentAnim.toFile = move.toFile;
	currentAnim.toRank = move.toRank;
	currentAnim.promotion = move.promotion;
	if (currentAnim.mover >= 0)
	{
		currentAnim.moverFrom = pieces[currentAnim.mover].pos;
		currentAnim.jumpArc = (pieces[currentAnim.mover].type == PIECE_KNIGHT);
	}
	else
		currentAnim.moverFrom = glm::vec3(worldX(move.fromFile), SURFACE_Y, worldZ(move.fromRank));
	currentAnim.moverTo = glm::vec3(worldX(move.toFile), SURFACE_Y, worldZ(move.toRank));

	if (occupancy[move.toFile][move.toRank] >= 0)
	{
		currentAnim.capturedPiece = occupancy[move.toFile][move.toRank];
		currentAnim.capturedFile = move.toFile;
		currentAnim.capturedRank = move.toRank;
	}
	else if (currentAnim.mover >= 0 && pieces[currentAnim.mover].type == PIECE_PAWN && move.fromFile != move.toFile && occupancy[move.toFile][move.fromRank] >= 0)
	{
		currentAnim.capturedPiece = occupancy[move.toFile][move.fromRank];
		currentAnim.capturedFile = move.toFile;
		currentAnim.capturedRank = move.fromRank;
	}
	if (currentAnim.capturedPiece >= 0)
	{
		currentAnim.capturedFrom = pieces[currentAnim.capturedPiece].pos;
		currentAnim.capturedTo = graveyardSlot(pieces[currentAnim.capturedPiece].color);
	}
	animActive = true;
}

void commitMove()
{
	if (!animActive)
		return;
	if (currentAnim.capturedPiece >= 0)
	{
		pieces[currentAnim.capturedPiece].captured = true;
		pieces[currentAnim.capturedPiece].pos = currentAnim.capturedTo;
		occupancy[currentAnim.capturedFile][currentAnim.capturedRank] = -1;
	}
	if (currentAnim.mover >= 0)
	{
		occupancy[currentAnim.fromFile][currentAnim.fromRank] = -1;
		pieces[currentAnim.mover].pos = currentAnim.moverTo;
		if (currentAnim.promotion != PIECE_NONE)
			pieces[currentAnim.mover].type = currentAnim.promotion;
		occupancy[currentAnim.toFile][currentAnim.toRank] = currentAnim.mover;
	}
	if (currentAnim.rook >= 0)
	{
		occupancy[currentAnim.rookFromFile][currentAnim.rookFromRank] = -1;
		pieces[currentAnim.rook].pos = currentAnim.rookTo;
		occupancy[currentAnim.rookToFile][currentAnim.rookToRank] = currentAnim.rook;
	}
	animActive = false;
}

glm::vec3 capturedAnimPos(float progress)
{
	float lift = 2.6f;
	glm::vec3 from = currentAnim.capturedFrom, to = currentAnim.capturedTo;
	if (progress < 0.34f)
	{
		float u = progress / 0.34f;
		return glm::vec3(from.x, from.y + lift * u, from.z);
	}
	else if (progress < 0.68f)
	{
		float u = (progress - 0.34f) / 0.34f;
		return glm::vec3(glm::mix(from.x, to.x, u), from.y + lift, glm::mix(from.z, to.z, u));
	}
	float u = (progress - 0.68f) / 0.32f;
	return glm::vec3(to.x, glm::mix(from.y + lift, to.y, u), to.z);
}

glm::vec3 renderPos(int pieceIndex)
{
	if (animActive)
	{
		float t = moveTimer / MOVE_DURATION;
		if (t > 1.0f)
			t = 1.0f;
		if (t < 0.0f)
			t = 0.0f;
		float smooth = t * t * (3.0f - 2.0f * t);
		if (pieceIndex == currentAnim.mover)
		{
			glm::vec3 pos = glm::mix(currentAnim.moverFrom, currentAnim.moverTo, smooth);
			if (currentAnim.jumpArc)
				pos.y += 2.3f * sinf(PI * t);
			return pos;
		}
		if (pieceIndex == currentAnim.rook)
			return glm::mix(currentAnim.rookFrom, currentAnim.rookTo, smooth);
		if (pieceIndex == currentAnim.capturedPiece)
			return capturedAnimPos(t);
	}
	return pieces[pieceIndex].pos;
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
	if (action == GLFW_PRESS && key != GLFW_KEY_ESCAPE)
		started = true;
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
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampVertical ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	return tex;
}

void initOpenGLProgram(GLFWwindow *window)
{
	glClearColor(0.03f, 0.03f, 0.07f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	spLit = new ShaderProgram("v_lit.glsl", NULL, "f_lit.glsl");
	spSky = new ShaderProgram("v_tex.glsl", NULL, "f_tex.glsl");
	spDepth = new ShaderProgram("v_depth.glsl", NULL, "f_depth.glsl");
	texSky = readTexture("sky.png", true);
	texWoodColor = readTexture("wood_color.png");
	texWoodNormal = readTexture("wood_normal.png");
	texWoodRough = readTexture("wood_rough.png");
	materialColorMap = texWoodColor;
	materialNormalMap = texWoodNormal;
	materialRoughMap = texWoodRough;
	texBoardColor = readTexture("board_color.png");
	texBoardNormal = readTexture("board_normal.png");
	texBoardRough = readTexture("board_rough.png");
	texWhiteColor = readTexture("marbleW_color.png");
	texWhiteNormal = readTexture("marbleW_normal.png");
	texWhiteRough = readTexture("marbleW_rough.png");
	texBlackColor = readTexture("marbleB_color.png");
	texBlackNormal = readTexture("marbleB_normal.png");
	texBlackRough = readTexture("marbleB_rough.png");

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_SIZE, SHADOW_SIZE, 0,
				 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
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

void drawLit(const Mesh &mesh, const glm::mat4 &M, const glm::vec4 &color)
{
	if (shadowPass)
	{
		spDepth->use();
		glm::mat4 mvp = lightSpaceMatrix * M;
		glUniformMatrix4fv(spDepth->u("MVP"), 1, false, glm::value_ptr(mvp));
		glEnableVertexAttribArray(spDepth->a("vertex"));
		glVertexAttribPointer(spDepth->a("vertex"), 3, GL_FLOAT, false, 0, mesh.positions.data());
		glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
		glDisableVertexAttribArray(spDepth->a("vertex"));
		return;
	}

	spLit->use();
	glUniformMatrix4fv(spLit->u("P"), 1, false, glm::value_ptr(matP));
	glUniformMatrix4fv(spLit->u("V"), 1, false, glm::value_ptr(matV));
	glUniformMatrix4fv(spLit->u("M"), 1, false, glm::value_ptr(M));
	glUniformMatrix4fv(spLit->u("lightSpaceMatrix"), 1, false, glm::value_ptr(lightSpaceMatrix));
	glUniform4fv(spLit->u("color"), 1, glm::value_ptr(color));
	glm::vec3 lightDir0 = glm::normalize(glm::vec3(mainLightPos) - glm::vec3(0.0f, 3.0f, 0.0f));
	glm::vec3 lightDir1 = glm::normalize(glm::vec3(fillLightPos) - glm::vec3(0.0f, 3.0f, 0.0f));
	glUniform3fv(spLit->u("lightDir0"), 1, glm::value_ptr(lightDir0));
	glUniform3fv(spLit->u("lightDir1"), 1, glm::value_ptr(lightDir1));
	glUniform1f(spLit->u("specStrength"), materialSpecular);
	glUniform3fv(spLit->u("lc0"), 1, glm::value_ptr(mainLightColor));
	glUniform3fv(spLit->u("lc1"), 1, glm::value_ptr(fillLightColor));
	glUniform3fv(spLit->u("ambientColor"), 1, glm::value_ptr(ambientColor));
	glUniform1i(spLit->u("shadowMap"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	bool textured = (materialTextured == 1 && !mesh.texCoords.empty());
	glUniform1i(spLit->u("useTex"), textured ? 1 : 0);
	glUniform2fv(spLit->u("uvScale"), 1, glm::value_ptr(materialUvScale));
	glUniform2fv(spLit->u("uvOffset"), 1, glm::value_ptr(materialUvOffset));
	glUniform1i(spLit->u("diffuseMap"), 0);
	glUniform1i(spLit->u("normalMap"), 2);
	glUniform1i(spLit->u("roughMap"), 3);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, materialColorMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, materialNormalMap);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, materialRoughMap);

	glEnableVertexAttribArray(spLit->a("vertex"));
	glVertexAttribPointer(spLit->a("vertex"), 3, GL_FLOAT, false, 0, mesh.positions.data());
	glEnableVertexAttribArray(spLit->a("normal"));
	glVertexAttribPointer(spLit->a("normal"), 3, GL_FLOAT, false, 0, mesh.normals.data());
	if (textured)
	{
		glEnableVertexAttribArray(spLit->a("texCoord0"));
		glVertexAttribPointer(spLit->a("texCoord0"), 2, GL_FLOAT, false, 0, mesh.texCoords.data());
		glEnableVertexAttribArray(spLit->a("tangent"));
		glVertexAttribPointer(spLit->a("tangent"), 3, GL_FLOAT, false, 0, mesh.tangents.data());
	}

	glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

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
	glm::mat4 M = translate(cameraPos.x, cameraPos.y, cameraPos.z) * scale(200.0f, 200.0f, 200.0f);
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
	materialSpecular = 0.5f;
	materialTextured = 1;
	materialColorMap = texWoodColor;
	materialNormalMap = texWoodNormal;
	materialRoughMap = texWoodRough;
	glm::vec4 tint(1.0f, 1.0f, 1.0f, 1.0f);

	materialUvScale = glm::vec2(3.0f, 3.0f);
	drawLit(mCube, worldRot * translate(0, 3.0f, 0) * scale(12.0f, 0.4f, 12.0f), tint);

	materialUvScale = glm::vec2(1.0f, 3.0f);
	float legHeight = 2.8f, legOffset = 5.2f;
	for (int sx = -1; sx <= 1; sx += 2)
		for (int sz = -1; sz <= 1; sz += 2)
			drawLit(mCube, worldRot * translate(sx * legOffset, legHeight * 0.5f, sz * legOffset) * scale(0.5f, legHeight, 0.5f), tint);

	materialTextured = 0;
	materialUvScale = glm::vec2(1.0f, 1.0f);
}

void drawBoard()
{
	materialSpecular = 0.85f;
	materialTextured = 1;
	materialColorMap = texBoardColor;
	materialNormalMap = texBoardNormal;
	materialRoughMap = texBoardRough;
	float top = 3.2f;

	materialUvScale = glm::vec2(3.0f, 3.0f);
	materialUvOffset = glm::vec2(0.0f, 0.0f);
	drawLit(mCube, worldRot * translate(0, top + 0.1f, 0) * scale(9.0f, 0.2f, 9.0f), glm::vec4(0.45f, 0.30f, 0.17f, 1.0f));

	glm::vec4 lightSquare(1.00f, 0.86f, 0.66f, 1.0f);
	glm::vec4 darkSquare(0.42f, 0.27f, 0.15f, 1.0f);
	float squareTop = top + 0.2f;
	materialUvScale = glm::vec2(1.0f, 1.0f);
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
		{
			glm::vec4 squareColor = ((f + r) % 2 == 0) ? darkSquare : lightSquare;
			materialUvOffset = glm::vec2(f * 0.137f + r * 0.219f, r * 0.137f + f * 0.219f);
			drawLit(mCube, worldRot * translate(worldX(f), squareTop + 0.05f, worldZ(r)) * scale(1.0f, 0.1f, 1.0f), squareColor);
		}

	materialTextured = 0;
	materialUvScale = glm::vec2(1.0f, 1.0f);
	materialUvOffset = glm::vec2(0.0f, 0.0f);
}

void drawKnight(const glm::mat4 &base, int color, const glm::vec4 &tint)
{
	glm::mat4 facing = base * rotateY(color == COLOR_WHITE ? 0.0f : PI);
	drawLit(mKnightBase, facing, tint);
	drawLit(mKnNeck, facing * translate(0, 0.45f, -0.02f) * rotateX(-0.30f), tint);
	drawLit(mKnHead, facing * translate(0, 0.78f, 0.10f) * rotateX(-0.10f), tint);
	drawLit(mKnSnout, facing * translate(0, 0.80f, 0.34f), tint);
	drawLit(mKnEar, facing * translate(-0.07f, 0.98f, -0.02f), tint);
	drawLit(mKnEar, facing * translate(0.07f, 0.98f, -0.02f), tint);
}

void drawPiece(const PieceInstance &piece, const glm::vec3 &pos, int textureSeed)
{
	materialSpecular = 0.7f;
	materialTextured = 1;
	if (piece.color == COLOR_WHITE)
	{
		materialColorMap = texWhiteColor;
		materialNormalMap = texWhiteNormal;
		materialRoughMap = texWhiteRough;
	}
	else
	{
		materialColorMap = texBlackColor;
		materialNormalMap = texBlackNormal;
		materialRoughMap = texBlackRough;
	}
	materialUvScale = glm::vec2(1.0f, 1.0f);
	materialUvOffset = glm::vec2(textureSeed * 0.37f, textureSeed * 0.61f);
	glm::vec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
	glm::mat4 base = worldRot * translate(pos.x, pos.y, pos.z);
	switch (piece.type)
	{
	case PIECE_PAWN:
		drawLit(mPawn, base, tint);
		break;
	case PIECE_BISHOP:
		drawLit(mBishop, base, tint);
		drawLit(mBall, base * translate(0, 1.24f, 0), tint);
		break;
	case PIECE_ROOK:
		drawLit(mRook, base, tint);
		for (int k = 0; k < 8; k++)
			drawLit(mMerlon, base * rotateY(k * PI / 4.0f) * translate(0.24f, 0.83f, 0.0f), tint);
		break;
	case PIECE_QUEEN:
		drawLit(mQueen, base, tint);
		for (int k = 0; k < 8; k++)
			drawLit(mSmallBall, base * rotateY(k * PI / 4.0f) * translate(0.25f, 1.40f, 0.0f), tint);
		drawLit(mSmallBall, base * translate(0, 1.52f, 0), tint);
		break;
	case PIECE_KING:
		drawLit(mKing, base, tint);
		drawLit(mCrossV, base * translate(0, 1.72f, 0), tint);
		drawLit(mCrossH, base * translate(0, 1.70f, 0), tint);
		break;
	case PIECE_KNIGHT:
		drawKnight(base, piece.color, tint);
		break;
	}
}

void renderObjects()
{
	materialTextured = 0;
	materialSpecular = 0.0f;
	drawLit(mGround, glm::mat4(1.0f), glm::vec4(0.10f, 0.18f, 0.19f, 1.0f));
	drawTable();
	drawBoard();
	for (int i = 0; i < (int)pieces.size(); i++)
		drawPiece(pieces[i], renderPos(i), i);
}

void drawScene(GLFWwindow *window)
{
	glm::vec3 lightTarget = glm::vec3(0.0f, 3.0f, 0.0f);
	glm::vec3 lightDir = glm::normalize(glm::vec3(mainLightPos) - lightTarget);
	glm::vec3 lightUp = (fabs(lightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	float lightDistance = glm::length(glm::vec3(mainLightPos) - lightTarget);
	float nearPlane = lightDistance - 25.0f;
	if (nearPlane < 1.0f)
		nearPlane = 1.0f;
	glm::mat4 lightView = glm::lookAt(glm::vec3(mainLightPos), lightTarget, lightUp);
	glm::mat4 lightProj = glm::ortho(-13.0f, 13.0f, -13.0f, 13.0f, nearPlane, lightDistance + 30.0f);
	lightSpaceMatrix = lightProj * lightView;

	shadowPass = true;
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
	renderObjects();
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shadowPass = false;
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec3 front = cameraFront();
	matV = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
	matP = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);

	drawSky();
	renderObjects();

	glfwSwapBuffers(window);
}

void updateGame(float dt)
{
	if (!playing || currentMove >= (int)game.size())
		return;
	moveTimer += dt;
	if (moveTimer >= MOVE_DURATION + MOVE_PAUSE)
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

	aspectRatio = 1920.0f / 1080.0f;
	window = glfwCreateWindow(1920, 1080, "Szachy 3D - odtwarzacz partii", NULL, NULL);
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

		if (started)
			updateGame(dt);
		drawScene(window);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
