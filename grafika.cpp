#include <iostream>

#define GL3_PROTOTYPES 1
#include <glew.h>
#include <SDL.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define WIDTH 800
#define HEIGHT 600

//------------------------------------------------------------------------------------------------------------------------------
//Globalne zmienne 

// kod zrodlowy shadera wierzcholkow
const GLchar* vertexSource =
"#version 150 core\n"
"in vec3 position;"
"in vec3 color;"
"in vec2 texcoord;"
"out vec3 Color;"
"out vec2 Texcoord;"
"uniform mat4 transformMatrix;"
"void main()"
"{"
"    gl_Position =  transformMatrix * vec4(position, 1.0);"
"    Color = color;"
"	 Texcoord = texcoord;"
"}";

// kod zrodlowy shadera fragmentow
const GLchar* fragmentSource =
"#version 150 core\n"
"in vec3 Color;"
"in vec2 Texcoord;"
"out vec4 outColor;"
"uniform sampler2D tex;"
"void main()"
"{"
"    outColor = texture(tex, Texcoord) * vec4(Color, 1.0);"
"}";


//------------------------------------------------------------------------------------------------------------------------------

GLint posAttrib, colAttrib, texAttrib;					//wskazniki atrybutow wierzcholkow
GLuint vertexShader, fragmentShader, shaderProgram;		//shadery


GLuint vao[2], vbo[4], ebo, tex;	// identyfikatory poszczegolnych obiektow (obiekty tablic wierzcholkow, buforow wierzcholkow, elementow, tekstury)

void CreateCylinder(const int n, GLfloat*& ver_cylinder, GLfloat*& col_cylinder, const float radius, const float height) {
	ver_cylinder = new GLfloat[n * 6];
	col_cylinder = new GLfloat[n * 6];
	float x, y;
	for (int i = 0; i < n * 2; i += 2) {
		x = cos(i * glm::radians(360.0f / (n - 1) / 2)) * radius - sin(i * glm::radians(360.0f / (n - 1) / 2)) * radius;
		y = cos(i * glm::radians(360.0f / (n - 1) / 2)) * radius + sin(i * glm::radians(360.0f / (n - 1) / 2)) * radius;
		ver_cylinder[i * 3] = x;
		ver_cylinder[i * 3 + 1] = 0.0f;
		ver_cylinder[i * 3 + 2] = y;
		ver_cylinder[(i + 1) * 3] = x;
		ver_cylinder[(i + 1) * 3 + 1] = height;
		ver_cylinder[(i + 1) * 3 + 2] = y;

		col_cylinder[i * 3] = i % 4 == 0? -1.0f : 0.0f;
		col_cylinder[i * 3 + 1] = 0.0f;
		col_cylinder[i * 3 + 2] = i % 4 == 0 ? 0.0f : -1.0f;
		col_cylinder[(i + 1) * 3] = i % 4 == 0 ? 1.0f : 0.0f;
		col_cylinder[(i + 1) * 3 + 1] = 0.0f;
		col_cylinder[(i + 1) * 3 + 2] = i % 4 == 0 ? 0.0f : 1.0f;
	}
}

//------------------------------------------------------------------------------------------------------------------------------

GLfloat ver_cube[] = {
	//dol
	-2.0f, 0.0f, -2.0f,
	2.0f, 0.0f, -2.0f,
	2.0f, 0.0f, 2.0f,
	-2.0f, 0.0f, 2.0f,
	//gora
	-2.0f, 4.0f, -2.0f,
	2.0f, 4.0f, -2.0f,
	2.0f, 4.0f, 2.0f,
	-2.0f, 4.0f, 2.0f,
};

GLfloat col_cube[] = { //kolory wierzcholkow podlogi
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
};

GLuint elements_cube[] = {
	0,1,2,
	2,3,0,

	4,5,6,
	6,7,4,

	0,5,1,
	0,4,5,

	1,6,2,
	1,5,6,

	2,7,3,
	2,6,7,

	3,4,7,
	3,0,4,
};

unsigned int cylinder_ver_count = 16; // ilosc punktow na okregu

GLfloat* ver_cylinder;

GLfloat* col_cylinder;

float pixels_floor[]{
	1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

//------------------------------------------------------------------------------------------------------------------------------

int init_shaders()
{
	// tworzenie i kompilacja shadera wierzcholkow
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cout << "Kompilacja shadera wierzcholkow NIE powiodla sie!\n";
		return 0;
	}

	// tworzenie i kompilacja shadera fragmentow
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cout << "Kompilacja shadera fragmentow NIE powiodla sie!\n";
		return 0;
	}

	// dolaczenie programow przetwarzajacych wierzcholki i fragmenty do programu cieniujacego
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// wskazniki atrybutow wierzcholkow
	posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);

	return 1;

}

//------------------------------------------------------------------------------------------------------------------------------

void create_objects()
{
	CreateCylinder(cylinder_ver_count, ver_cylinder, col_cylinder, 1.0f, 6.0f);

	// generowanie obiektow
	glGenVertexArrays(2, vao);  // obiekt tablicy wierzcholkow, dla kazdego obiektu (np. dla podlogi) mamy jedna tablice
	glGenBuffers(4, vbo);		// obiekty buforow wierzcholkow, dla kazdego typu atrubutow kazdego obiektu mamy jeden bufor (np. bufor dla kolorow podlogi, bufor dla wspolrzednych podlogi itd.)
	glGenBuffers(1, &ebo);		// obiekt bufora elementow (ten sam bufor mozna wykorzystac zarowno dla podlogi jak i sciany)

	// cube vao[0]

	glBindVertexArray(vao[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);	// bufor wspolrzednych wierzcholkow trojkata
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_cube), ver_cube, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);	// bufor kolorow wierzcholkow trojkata
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_cube), col_cube, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements_cube), elements_cube, GL_STATIC_DRAW);

	glBindVertexArray(vao[1]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);	// bufor wspolrzednych wierzcholkow trojkata
	glBufferData(GL_ARRAY_BUFFER, cylinder_ver_count * 6 * 4, ver_cylinder, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);	// bufor kolorow wierzcholkow trojkata
	glBufferData(GL_ARRAY_BUFFER, cylinder_ver_count * 6 * 4, col_cylinder, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);
}

//------------------------------------------------------------------------------------------------------------------------------

void configure_texture()
{
	glGenTextures(1, &tex);		// obiekt tekstury
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels_floor); // ladowanie do tekstury tablicy pikseli
}

//------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char ** argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window* window = SDL_CreateWindow("OpenGL", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_Event windowEvent;

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cout << "ERROR" << std::endl;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // GL_ALWAYS)

	if(!init_shaders())
		return 0;

	create_objects();

	configure_texture();

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;
	
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 20.0f);		 //macierz rzutowania perspektywicznego
	glm::mat4 viewMatrix;  //macierz widoku
	glm::mat4 transformMatrix; //macierz wynikowa
	float rotationAngle = 0.0f;

	GLint transformMatrixUniformLocation = glGetUniformLocation(shaderProgram, "transformMatrix");

	int top_view = false; //zmienna okreslajaca czy patrzymy na scene z gory

	glm::vec3 position = glm::vec3(-2.0f, 10.0f, 12.5f); //poczatkowe polozenie kamery
	glm::vec3 direction = glm::vec3(0.5f, -0.5f, -1.0f); //poczatkowy kierunek, w ktorym kamera jest skierowana


	while (true)
	{
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());

		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_KEYUP &&
				windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
			if (windowEvent.type == SDL_QUIT) break;


			if (windowEvent.type == SDL_KEYDOWN)
			{
				switch (windowEvent.key.keysym.sym)
				{
				case SDLK_SPACE:
					top_view = !top_view;
					break;

				//case SDLK_UP:
				
				//case SDLK_DOWN:
					
				//case SDLK_LEFT:
					
				//case SDLK_RIGHT:
				
				}

			}

		}
	
		if (top_view) //patrzymy z gory
			viewMatrix = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	
		else //patrzymy z miejsca, w ktorym jest obserwator 
			viewMatrix = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	
			
		transformMatrix = projectionMatrix * viewMatrix;				// wynikowa macierz transformacji
		glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));	// macierz jako wejœciowa zmienna dla shadera wierzcholkow

		
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);	// szare tlo
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		int a = 6, b = 8;
		
		rotationAngle += 0.25f * deltaTime;
		
		for (int i = 0; i <= a; i++) { //pionowe
			for (int j = 0; j <= b; j++) { //poziome
				transformMatrix = projectionMatrix * viewMatrix;
				transformMatrix = glm::scale(transformMatrix, glm::vec3(0.25f, 0.25f, 0.25f));
				transformMatrix = glm::translate(transformMatrix, glm::vec3(j * 8.0f, i * 8.0f, 0.0f));
				glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));
				glBindVertexArray(vao[0]);
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

				if (i != a) {
					transformMatrix = projectionMatrix * viewMatrix;
					transformMatrix = glm::scale(transformMatrix, glm::vec3(0.25f, 0.25f, 0.25f));
					transformMatrix = glm::translate(transformMatrix, glm::vec3(j * 8.0f, 4.0f + i * 8.0f, 0.0f));
					transformMatrix = glm::rotate(transformMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));
					glBindVertexArray(vao[1]);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, cylinder_ver_count * 2);
				}

				if (j != b) {
					transformMatrix = projectionMatrix * viewMatrix;
					transformMatrix = glm::scale(transformMatrix, glm::vec3(0.25f, 0.25f, 0.25f));
					transformMatrix = glm::translate(transformMatrix, glm::vec3(j * 8.0f, 2.0f + i * 8.0f, 0.0f));
					transformMatrix = glm::rotate(transformMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					transformMatrix = glm::rotate(transformMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
					transformMatrix = glm::rotate(transformMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));
					glBindVertexArray(vao[1]);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, cylinder_ver_count * 2);
				}
			}
		}

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(4, vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteTextures(1, &tex);
	glDeleteVertexArrays(2, vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();

	return 0;
}