/*
Niniejszy program jest wolnym oprogramowaniem; mo¿esz go
rozprowadzaæ dalej i / lub modyfikowaæ na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundacjê Wolnego
Oprogramowania - wed³ug wersji 2 tej Licencji lub(wed³ug twojego
wyboru) którejœ z póŸniejszych wersji.

Niniejszy program rozpowszechniany jest z nadziej¹, i¿ bêdzie on
u¿yteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyœlnej
gwarancji PRZYDATNOŒCI HANDLOWEJ albo PRZYDATNOŒCI DO OKREŒLONYCH
ZASTOSOWAÑ.W celu uzyskania bli¿szych informacji siêgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnoœci¹ wraz z niniejszym programem otrzyma³eœ te¿ egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeœli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include "constants.h"
#include "allmodels.h"
#include <sstream>
#include <tuple>


std::vector<std::vector<int>> tileData;
Models::Sphere ball(3, 36, 36);
int titlesCountHori, titlesCountVerti;
float tileLength, tileWidth, cubePos, paddle_position, camera_position;
bool move_left, move_right, ball_stuck, camera_up, camera_down;
glm::vec2 ball_vel;
glm::vec2 ball_position;
std::vector<glm::vec4> blocks;


int levelCode = 1;
const glm::vec2 INITIAL_BALL_VELOCITY(30.0f, 35.0f);
const GLfloat INITIAL_PADDLE_POSITION = 100.0f;
const float paddle_speed = 150.0f;
const float paddle_size = 30.0f;
bool camera_fixed = false;

enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
};
void loadBlocks() {
	for (int i = 1; i <= titlesCountHori; i++) {
		for (int j = 0; j < titlesCountVerti; j++) {
			if (tileData[j][i - 1] != 0) {
				glm::vec4 tmp_vec = glm::vec4(i*tileWidth - tileWidth / 2, 150.0f - 2 * j * tileLength - tileLength, tileData[j][i - 1], 1);
				blocks.push_back(tmp_vec);
			}
		}
	}
}

//Procedura obs³ugi b³êdów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void key_callback(GLFWwindow* window, int key,
	int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) move_left = true;
		if (key == GLFW_KEY_RIGHT) move_right = true;
		if (key == GLFW_KEY_UP) ball_stuck = false;
		if (key == GLFW_KEY_W) camera_up = true;
		if (key == GLFW_KEY_S) camera_down = true;
		if (key == GLFW_KEY_SPACE) camera_fixed = !camera_fixed;
}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) move_left = false;
		if (key == GLFW_KEY_RIGHT) move_right = false;
		if (key == GLFW_KEY_W) camera_up = false;
		if (key == GLFW_KEY_S) camera_down = false;
	}
	
}


Direction VectorDirection(glm::vec2 target)
{
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f),	
		glm::vec2(1.0f, 0.0f),	
		glm::vec2(0.0f, -1.0f),
		glm::vec2(-1.0f, 0.0f)
	};
	GLfloat max = 0.0f;
	GLuint best_match = -1;
	for (GLuint i = 0; i < 4; i++)
	{
		GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
		if (dot_product > max)
		{
			max = dot_product;
			best_match = i;
		}
	}
	return (Direction)best_match;
}

//KOLIZJE MIÊDZY KULK¥ A KLOCKIEM
typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;
Collision CheckCollision(glm::vec2 &ball, glm::vec4 &two)
{
	
	glm::vec2 center(ball.x, ball.y);
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(tileWidth / 2, tileLength);
	glm::vec2 aabb_center(two.x, two.y);
	// Get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Add clamped value to AABB_center and we get the value of box closest to circle
	glm::vec2 closest = aabb_center + clamped;
	// Retrieve vector between center circle and closest point AABB and check if length <= radius
	difference = closest - center;
	//return glm::length(difference) < 3.0f;

	if (glm::length(difference) <= 3.0f)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

Collision CheckCollisionWithPaddle(glm::vec2 &ball, glm::vec2 &two) // AABB - Circle collision
{
	// Get center point circle first 
	glm::vec2 center(ball.x, ball.y);
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(15.0f, 2.0f);
	glm::vec2 aabb_center(two.x, two.y);
	// Get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Add clamped value to AABB_center and we get the value of box closest to circle
	glm::vec2 closest = aabb_center + clamped;
	// Retrieve vector between center circle and closest point AABB and check if length <= radius
	difference = closest - center;
	//return glm::length(difference) < 3.0f;

	if (glm::length(difference) <= 3.0f)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

void DoCollisions()
{
	for (int i = 0; i < blocks.size(); i++)
	{
		if (blocks[i].w != 0) {
			Collision collision = CheckCollision(ball_position, blocks[i]);
			if (std::get<0>(collision)) // If collision is true
			{
				// Destroy block if not solid
				blocks[i].w = 0;
				// Collision resolution
				Direction dir = std::get<1>(collision);
				glm::vec2 diff_vector = std::get<2>(collision);
				if (dir == LEFT || dir == RIGHT) // Horizontal collision
				{
					ball_vel.x = -ball_vel.x; // Reverse horizontal velocity
											  // Relocate
					GLfloat penetration = 3.0f - std::abs(diff_vector.x);
					if (dir == LEFT)
						ball_position.x += penetration; // Move ball to right
					else
						ball_position.x -= penetration; // Move ball to left;
				}
				else // Vertical collision
				{
					ball_vel.y = -ball_vel.y; // Reverse vertical velocity
											  // Relocate
					GLfloat penetration = 3.0f - std::abs(diff_vector.y);
					if (dir == UP)
						ball_position.y -= penetration; // Move ball back up
					else
						ball_position.y += penetration; // Move ball back down
				}
			}
		}
	}
	//KOLIZJE Z PADDLEM
	//s¹ jako osobna funkcja bo w rozmiar jest na sztywno
	Collision result = CheckCollisionWithPaddle(ball_position, glm::vec2(paddle_position, 2.0f));
	if ( !ball_stuck && std::get<0>(result))
	{
		GLfloat centerBoard = paddle_position;
		GLfloat distance = (ball_position.x) - centerBoard;
		GLfloat percentage = distance / paddle_size;
		GLfloat strength = 2.0f;
		glm::vec2 oldVelocity = ball_vel;
		ball_vel.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		ball_vel.y = abs(ball_vel.y);
		ball_vel = glm::normalize(ball_vel) * glm::length(oldVelocity);
	}

}

//Procedura inicjuj¹ca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który nale¿y wykonaæ raz, na pocz¹tku programu************
	glfwSetKeyCallback(window, key_callback);
	glClearColor(0, 0, 0, 1); //Czyœæ ekran na czarno
	glEnable(GL_LIGHTING); //W³¹cz tryb cieniowania
	glEnable(GL_LIGHT0); //W³¹cz domyslne œwiat³o
	glEnable(GL_DEPTH_TEST); //W³¹cz u¿ywanie Z-Bufora
	glEnable(GL_COLOR_MATERIAL); //glColor3d ma modyfikowaæ w³asnoœci materia³u
	
}

//Procedura rysuj¹ca zawartoœæ sceny
void drawScene(GLFWwindow* window) {
	//************Tutaj umieszczaj kod rysuj¹cy obraz******************
	
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); //Wykonaj czyszczenie bufora kolorów

	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 V;
	//KAMERA ALBO NA PADDLE ALBO NA SRODEK
	if (camera_fixed) {
		V = glm::lookAt(
			glm::vec3(100.0f, camera_position, 5.0f),
			glm::vec3(100.0f, 0.0f, 50.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else {
		V = glm::lookAt(
			glm::vec3(paddle_position, camera_position, 5.0f),
			glm::vec3(paddle_position, 0.0f, 50.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
	}
	
	glm::mat4 P = glm::perspective(100 * PI / 180, 4.0f/3.0f, 1.0f, 300.0f); //Wylicz macierz rzutowania

	//Za³aduj macierze do OpenGL
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(P));
	glMatrixMode(GL_MODELVIEW);

	//RYSOWANIE KLOCKOW
	glColor3d(0.5f, 0.5f, 0.5f);
	for (glm::vec4 &klocuch : blocks) {
		if (klocuch.w == 1) {
			glm::mat4 M = glm::mat4(1.0f);
			M = glm::translate(M, glm::vec3(klocuch.x, 0.0f, klocuch.y));
			M = glm::scale(M, glm::vec3(tileWidth / 2, 2.0f, tileLength));
			glLoadMatrixf(glm::value_ptr(V*M));
			if (klocuch.z == 2)
				glColor3d(0.2f, 0.6f, 1.0f);
			else if (klocuch.z == 3)
				glColor3d(0.0f, 0.7f, 0.0f);
			else if (klocuch.z == 4)
				glColor3d(0.8f, 0.8f, 0.4f);
			else if (klocuch.z == 5)
				glColor3d(1.0f, 0.5f, 0.0f);
			else if (klocuch.z == 1)
				glColor3d(0.0f, 0.25f, 0.5f);
			Models::cube.drawSolid();
			glColor3d(0.5f, 0.5f, 0.5f);
		}
	}

	//BARIERKI
	glColor3d(0.5f, 0.5f, 0.5f);
	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(-2.5f, 0.0f, 77.5f));
	M = glm::scale(M, glm::vec3(2.5f, 5.0f, 77.5f));
	glLoadMatrixf(glm::value_ptr(V*M));
	Models::cube.drawSolid();

	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(100.0f, 0.0f, 152.5f));
	M = glm::scale(M, glm::vec3(100.0f, 5.0f, 2.5f));
	glLoadMatrixf(glm::value_ptr(V*M));
	Models::cube.drawSolid();

	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(202.5f, 0.0f, 77.5f));
	M = glm::scale(M, glm::vec3(2.5f, 5.0f, 77.5f));
	glLoadMatrixf(glm::value_ptr(V*M));
	Models::cube.drawSolid();

	//PADDLE
	glColor3d(0.8f, 0.8f, 0.8f);
	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(paddle_position, 0.0f, 2.0f));
	M = glm::scale(M, glm::vec3(paddle_size/2, 5.0f, 2.0f));
	glLoadMatrixf(glm::value_ptr(V*M));
	Models::cube.drawSolid();

	//BALL
	glColor3d(0.8f, 0.8f, 0.0f);
	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(ball_position.x, 0.0f, ball_position.y));
	glLoadMatrixf(glm::value_ptr(V*M));
	ball.drawSolid();
	
	glfwSwapBuffers(window);

}

int main(void)
{
	GLFWwindow* window; //WskaŸnik na obiekt reprezentuj¹cy okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurê obs³ugi b³êdów

	if (!glfwInit()) { //Zainicjuj bibliotekê GLFW
		fprintf(stderr, "Nie mo¿na zainicjowaæ GLFW.\n");
		exit(EXIT_FAILURE); 
	}

	window = glfwCreateWindow(1024, 768, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL. 

	if (!window) //Je¿eli okna nie uda³o siê utworzyæ, to zamknij program
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje siê aktywny i polecenia OpenGL bêd¹ dotyczyæ w³aœnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekê GLEW
		fprintf(stderr, "Nie mo¿na zainicjowaæ GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjuj¹ce

	glfwSetTime(0); //Wyzeruj licznik czasu

	//£ADOWANIE POZIOMU Z PLIKU
	using namespace std;
	fstream plik;
	string line;
	int tileCode;
	
	/* WCZYTYWANIE Z KONSOLI ALE TO S£ABE
	cout << "Wybierz poziom z zakresu 1-4" << endl;
	while (levelCode < 1 || levelCode > 4) {
		cin >> levelCode;
	}
	*/
	string filename;
	filename = to_string(levelCode) + ".lvl";

	ifstream myFile(filename);
	if (myFile.is_open()) {
		while (getline(myFile, line)) {
			istringstream iss(line);
			vector<int> row;
			while (iss >> tileCode) {
				row.push_back(tileCode);
			}
			tileData.push_back(row);
		}
	}
	//USTALENIE ROZMIARU POJEDYNCZEGO KLOCKA
	tileWidth = 200.0f / tileData[0].size();
	tileLength = 40.0f / tileData.size();
	titlesCountHori = tileData[0].size();
	titlesCountVerti = tileData.size();


	//£ADOWANIE POZYCJI Z TILEDATA DO BLOCKS
	//¿eby mieæ wszystkie pozycje w jednym miejscu
	loadBlocks();

	//USTAWIANIE WARTOŒCI DOMYŒLNYCH
	paddle_position = INITIAL_PADDLE_POSITION;
	ball_position.x = INITIAL_PADDLE_POSITION;
	ball_position.y = 8.0f;
	ball_vel = INITIAL_BALL_VELOCITY;
	ball_stuck = true;
	camera_position = 80.0f;
	
	//G³ówna pêtla
	while (!glfwWindowShouldClose(window)) //Tak d³ugo jak okno nie powinno zostaæ zamkniête
	{
		GLfloat dt = glfwGetTime();
		//PORUSZANIE PADDLEM
		if (move_left) {
			if (paddle_position <= 200.0f - paddle_size/2) {
				paddle_position += paddle_speed*dt;
				if(ball_stuck)
					ball_position.x += paddle_speed*dt;
			}
		}
		if (move_right) {
			if (paddle_position >= paddle_size/2) {
				paddle_position -= paddle_speed*dt;
				if(ball_stuck)
					ball_position.x -= paddle_speed*dt;
			}
		}
		//PORUSZANIE PILKI JESLI NIE JEST PRZYKLEJONA
		if (!ball_stuck)
		{
			ball_position += ball_vel * dt;
			ball_position += ball_vel * dt;
			if (ball_position.x <= 0.0f)
			{
				ball_vel.x = -ball_vel.x;
				ball_position.x = 0.0f;
			}
			else if (ball_position.x + 3.0f >= 200.0f)
			{
				ball_vel.x = -ball_vel.x;
				ball_position.x = 197.0f;
			}
			if (ball_position.y >= 150.0f)
			{
				ball_vel.y = -ball_vel.y;
				ball_position.y = 150.0f;
			}

		}
		//PORUSZANIE KAMERA GORA-DOL
		if (camera_up)
			camera_position += 20.0f * dt;
		if (camera_down)
			camera_position -= 20.0f * dt;
		//KOLIZJE
		DoCollisions();
		//RESET GRY JESLI PILKA SPADNIE PONIZEJ y = 0
		if (ball_position.y < 0.0f) {
			blocks.clear();
			loadBlocks();
			paddle_position = INITIAL_PADDLE_POSITION;
			ball_position.x = INITIAL_PADDLE_POSITION;
			ball_position.y = 8.0f;
			ball_vel = INITIAL_BALL_VELOCITY;
			ball_stuck = true;

		}

		glfwSetTime(0); //Wyzeruj licznik czasu
		drawScene(window); //Wykonaj procedurê rysuj¹c¹
		glfwPollEvents(); //Wykonaj procedury callback w zaleznoœci od zdarzeñ jakie zasz³y.
	}

	glfwDestroyWindow(window); //Usuñ kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajête przez GLFW
	exit(EXIT_SUCCESS);
}