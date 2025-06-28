#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>
#include <map>
#include <vector>
#include <string>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
// ================================
// NOVO: Leitura do mapa por Mapa.txt
// ================================

#include <fstream>

enum TileType {
    WALKABLE,
    DANGEROUS,
    NON_WALKABLE,
    WALKED,
    PLAYER,
    COIN,
    UNKNOWN // fallback para qualquer símbolo não definido
};

struct Pos {
    int row, col;
};

// vector<vector<TileType>> tilemap;
int TILEMAP_WIDTH = 0, TILEMAP_HEIGHT = 0;
int COIN_LINE = 0, COIN_COLUMN = 0;

int selectedTileMapLine = 1, selectedTileMapColumn = 1;

std::vector<std::vector<TileType>> tilemap;
std::map<char, TileType> tileTypeMap;
Pos playerPos;
std::vector<Pos> coinPositions;


void carregarMapaTxt(const string& path,
                     vector<vector<TileType>>& tilemap,
                     map<char, TileType>& tileTypeMap,
                     Pos& playerPos,
                     vector<Pos>& coinPositions) 
{
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o arquivo do mapa: " << path << "\n";
        exit(1);
    }

    int width, height;
    file >> width >> height;
    file.ignore(); // Avança para próxima linha

    // Lê linhas do mapa
    vector<string> linhasMapa;
    for (int i = 0; i < height; ++i) {
        string linha;
        getline(file, linha);
        linhasMapa.push_back(linha);
    }

    // Pula linha separadora '#'
    string line;
    while (getline(file, line)) {
        if (!line.empty() && line[0] == '#') break;
    }

    // Lê as definições de tipos de tile
    tileTypeMap.clear();
    while (getline(file, line)) {
        if (line.empty()) continue;
        // Ignora possíveis comentários no futuro (opcional)
        if (line[0] == '#') continue;

        // Esperado: símbolo:TIPO
        if (line.size() < 3 || line[1] != ':') continue;
        char symbol = line[0];
        string type = line.substr(2);

        if      (type == "WALKABLE")      tileTypeMap[symbol] = WALKABLE;
        else if (type == "DANGEROUS")     tileTypeMap[symbol] = DANGEROUS;
        else if (type == "NON_WALKABLE")  tileTypeMap[symbol] = NON_WALKABLE;
        else if (type == "WALKED")        tileTypeMap[symbol] = WALKED;
        else if (type == "PLAYER")        tileTypeMap[symbol] = PLAYER;
        else if (type == "COIN")          tileTypeMap[symbol] = COIN;
        else                              tileTypeMap[symbol] = UNKNOWN; // fallback
    }

    // Cria o tilemap e encontra o player e as moedas
    tilemap.resize(height, vector<TileType>(width, UNKNOWN));
    coinPositions.clear();

    for (int i = 0; i < height; ++i) {
        const string& linha = linhasMapa[i];
        for (int j = 0; j < width; ++j) {
            char c = linha[j];
            TileType tipo = tileTypeMap.count(c) ? tileTypeMap[c] : UNKNOWN;
            tilemap[i][j] = tipo;

            if (tipo == PLAYER) {
                playerPos = {i, j};
                // Substitui a posição inicial do player por piso caminhável no tilemap:
                tilemap[i][j] = tileTypeMap.count('0') ? tileTypeMap['0'] : WALKABLE;
            }
            if (tipo == COIN) {
                coinPositions.push_back({i, j});
                // Também transforma a moeda em piso caminhável (se for o caso)
                tilemap[i][j] = tileTypeMap.count('0') ? tileTypeMap['0'] : WALKABLE;
            }
        }
    }
}

struct Sprite
{
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions; // tamanho do frame

    bool isAnimated;
    float ds, dt;
    int iAnimation, iFrame;
	int nAnimations, nFrames;
    bool isAlive = true;
    bool isCollect = false;
};

Sprite principal;
Sprite coin;

struct Tile
{
    GLuint VAO;
    GLuint texID; // de qual tileset
    int iTile;    // indice dele no tileset
    vec3 position;
    vec3 dimensions; // tamanho do losango 2:1
    float ds, dt;
    int iAnimation, iFrame;
    int nAnimations, nFrames;
};

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
int setupTile(int nTiles, float &ds, float &dt);
int loadTexture(string filePath, int &width, int &height);
void desenharMapa(GLuint shaderID);
bool isTileInArray(TileType tileId, const TileType tileArray[], int arraySize);
void finalizarJogo();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1200, HEIGHT = 800;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 out vec2 tex_coord;
 uniform mat4 model;
 uniform mat4 projection;
 void main()
 {
	tex_coord = vec2(texc.s, 1.0 - texc.t);
	gl_Position = projection * model * vec4(position, 1.0);
 }
 )";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
 #version 400
 in vec2 tex_coord;
 out vec4 color;
 uniform sampler2D tex_buff;
 uniform vec2 offsetTex;

 void main()
 {
	 color = texture(tex_buff,tex_coord + offsetTex);
 }
 )";

vector<Tile> tileset;

const TileType NOT_WALKABLE_TILES[] = { NON_WALKABLE, WALKED };
const TileType DANGEROUS_TILES[] = { DANGEROUS };
const int NUM_NOT_WALKABLE_TILES = sizeof(NOT_WALKABLE_TILES) / sizeof(TileType);
const int NUM_DANGEROUS_TILES = sizeof(DANGEROUS_TILES) / sizeof(TileType);

const int WALKED_TILE = 6;

// Função MAIN
int main()
{

    carregarMapaTxt("../src/ExemplosMoodle/M6_Material/Map.txt", tilemap, tileTypeMap, playerPos, coinPositions);
    if (tilemap.empty()) {
        cerr << "Mapa não carregado corretamente.\n";
        exit(1);
    }

    cout << "Mapa lido! Linhas: " << tilemap.size() << endl;
    cout << "Colunas da primeira linha: " << tilemap[0].size() << endl;
    cout << "Player em: (" << playerPos.row << ", " << playerPos.col << ")" << endl;
    cout << "Qtde de moedas: " << coinPositions.size() << endl;
    if (!coinPositions.empty()) {
        cout << "Primeira moeda em: (" << coinPositions[0].row << ", " << coinPositions[0].col << ")" << endl;
    }


    // Inicialização da GLFW
    glfwInit();

    // Muita atenção aqui: alguns ambientes não aceitam essas configurações
    // Você deve adaptar para a versão do OpenGL suportada por sua placa
    // Sugestão: comente essas linhas de código para desobrir a versão e
    // depois atualize (por exemplo: 4.5 com 4 e 5)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Ativa a suavização de serrilhado (MSAA) com 8 amostras por pixel
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Essencial para computadores da Apple
    // #ifdef __APPLE__
    //	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // #endif

    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade vivencial - M6", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    // Obtendo as informações de versão
    const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
    const GLubyte *version = glGetString(GL_VERSION);   /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Carregando uma textura
    int imgWidth, imgHeight;
    // GLuint texID = loadTexture("../assets/sprites/Vampires1_Walk_full.png",imgWidth,imgHeight);
    GLuint texID = loadTexture("../assets/tilesets/tilesetIso.png", imgWidth, imgHeight);

    GLuint principalTexID = loadTexture("../assets/sprites/Vampires1_Walk_full.png", imgWidth, imgHeight);
    // Gerando um buffer simples, com a geometria de um triângulo
    principal.isAnimated = true;
    principal.nAnimations = 4;
	principal.nFrames = 6;
	principal.VAO = setupSprite(principal.nAnimations, principal.nFrames, principal.ds, principal.dt);
    principal.position = vec3(400.0, 150.0, 0.0);
    principal.dimensions = vec3(75, 75, 1.0);
    principal.texID = principalTexID;
    principal.iAnimation = 1;
	principal.iFrame = 0;

    GLuint cointTexID = loadTexture("../assets/sprites/coin.png", imgWidth, imgHeight);
    // Gerando um buffer simples, com a geometria de um triângulo
    coin.isAnimated = false;
    coin.nAnimations = 1;
    coin.nFrames = 1;
    coin.ds = 1.0f;
    coin.dt = 1.0f;
    coin.VAO = setupSprite(1, 1, coin.ds, coin.dt);
    coin.position = vec3(0.0, 0.0, 0.0);
    coin.dimensions = vec3(35, 35, 1.0);
    coin.texID = cointTexID;

    // Configura o tileset - conjunto de tiles do mapa
    for (int i = 0; i < 7; i++)
    {
        Tile tile;
        tile.dimensions = vec3(75, 45, 1.0);
        tile.iTile = i;
        tile.texID = texID;
        tile.VAO = setupTile(7, tile.ds, tile.dt);
        tileset.push_back(tile);
    }

    glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

    double prev_s = glfwGetTime();  // Define o "tempo anterior" inicial.
    double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

    float colorValue = 0.0;

    // Ativando o primeiro buffer de textura do OpenGL
    glActiveTexture(GL_TEXTURE0);

    // Criando a variável uniform pra mandar a textura pro shader
    glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

    // Matriz de projeção paralela ortográfica
    mat4 projection = ortho(0.0, 1200.0, 0.0, 800.0, -1.0, 1.0);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST); // Habilita o teste de profundidade
    glDepthFunc(GL_ALWAYS);  // Testa a cada ciclo

    glEnable(GL_BLEND);                                // Habilita a transparência -- canal alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Seta função de transparência

    double lastTime = 0.0;
    double deltaT = 0.0;
    double currTime = glfwGetTime();
    double FPS = 12.0;

    tilemap[selectedTileMapLine - 1][selectedTileMapColumn - 1] == WALKED_TILE;

    std::cout << "Bem vindo!" << std::endl;
    std::cout << "O objetivo deste jogo é coletar a moeda e chegar ao tile preto, nessa ordem" << std::endl;
    std::cout << "Cuidado! Você pode morrer na lava!" << std::endl;

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
        glfwPollEvents();

        if (!principal.isAlive)
        {
            std::cout << "Você morreu!" << std::endl;
            glfwTerminate();
            return 0;
        }

        // Limpa o buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        // Desenhar o mapa
        desenharMapa(shaderID);

        //---------------------------------------------------------------------
        // Desenho do principal
        // Matriz de transformaçao do objeto - Matriz de modelo
        mat4 model = mat4(1); // matriz identidade

        vec2 offsetTex;

        currTime = glfwGetTime();
		deltaT = currTime - lastTime;

		if (principal.isAnimated) {
            if (deltaT >= 1.0 / FPS)
		    {
		    	principal.iFrame = (principal.iFrame + 1) % principal.nFrames; // incremento "circular"
		    	lastTime = currTime;
		    }

		    offsetTex.s = principal.iFrame * principal.ds;
		    offsetTex.t = (principal.iAnimation) * principal.dt;
		    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
        }

        float tile_iso_width = tileset[0].dimensions.x;
        float tile_iso_height = tileset[0].dimensions.y;

        float x0 = 615;
        float y0 = 100;

        float x = x0 + (selectedTileMapColumn - selectedTileMapLine) * (tile_iso_width / 2.0f);
        float y = (y0 + (selectedTileMapLine + selectedTileMapColumn) * (tile_iso_height / 2.0f)) + (tile_iso_height / 2.0f) - (principal.dimensions.y / 2.0f);

        vec3 position = vec3(x, y, 0.0);

        model = translate(model, position);
        model = rotate(model, radians(0.0f), vec3(0.0, 0.0, 1.0));
        model = scale(model, principal.dimensions);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

        glBindVertexArray(principal.VAO);              // Conectando ao buffer de geometria
        glBindTexture(GL_TEXTURE_2D, principal.texID); // Conectando ao buffer de textura

        // Chamada de desenho - drawcall
        // Poligono Preenchido - GL_TRIANGLES
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //---------------------------------------------------------------------------

        //---------------------------------------------------------------------
        // Desenho da coin
        // Matriz de transformaçao do objeto - Matriz de modelo
        
        
        if (!coin.isCollect) {

        	glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 1, 1);

            model = mat4(1); // matriz identidade

            float x0Coin = 615;
            float y0Coin = 80;

            float xCoin = x0Coin + (COIN_COLUMN - COIN_LINE) * (tile_iso_width / 2.0f);
            float yCoin = (y0Coin + (COIN_LINE + COIN_COLUMN) * (tile_iso_height / 2.0f)) + (tile_iso_height / 2.0f) - (coin.dimensions.y / 2.0f);

            vec3 positionCoin = vec3(xCoin, yCoin, 0.0);

            model = translate(model, positionCoin);
            model = rotate(model, radians(0.0f), vec3(0.0, 0.0, 1.0));
            model = scale(model, coin.dimensions);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

            glBindVertexArray(coin.VAO);              // Conectando ao buffer de geometria
            glBindTexture(GL_TEXTURE_2D, coin.texID); // Conectando ao buffer de textura

            // Chamada de desenho - drawcall
            // Poligono Preenchido - GL_TRIANGLES
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        //---------------------------------------------------------------------------

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    int possibleTileMapLine = selectedTileMapLine;
    int possibleTileMapColumn = selectedTileMapColumn;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_A) { possibleTileMapLine += 1; possibleTileMapColumn -= 1; principal.iAnimation = 3; }
        if (key == GLFW_KEY_D) { possibleTileMapLine -= 1; possibleTileMapColumn += 1; principal.iAnimation = 4; }
        if (key == GLFW_KEY_W) { possibleTileMapLine += 1; possibleTileMapColumn += 1; principal.iAnimation = 2; }
        if (key == GLFW_KEY_S) { possibleTileMapLine -= 1; possibleTileMapColumn -= 1; principal.iAnimation = 1; }
        if (key == GLFW_KEY_E) { possibleTileMapColumn += 1; principal.iAnimation = 4; }
        if (key == GLFW_KEY_Q) { possibleTileMapLine += 1; principal.iAnimation = 3; }
        if (key == GLFW_KEY_C) { possibleTileMapLine -= 1; principal.iAnimation = 4; }
        if (key == GLFW_KEY_Z) { possibleTileMapColumn -= 1; principal.iAnimation = 3; }

        // Cuidado: índices da matriz vão de 1 a TILEMAP_HEIGHT (inclusive), mas acesso é -1
        int newLine = possibleTileMapLine - 1;
        int newCol  = possibleTileMapColumn - 1;

        // Checagem de limites ABSOLUTAMENTE obrigatória
        if (newLine >= 0 && newLine < TILEMAP_HEIGHT &&
            newCol  >= 0 && newCol  < TILEMAP_WIDTH)
        {
            // Aqui o acesso é seguro
            if (!isTileInArray(tilemap[newLine][newCol], NOT_WALKABLE_TILES, NUM_NOT_WALKABLE_TILES))
            {
                selectedTileMapLine = possibleTileMapLine;
                selectedTileMapColumn = possibleTileMapColumn;
            }

            // Verifica se morreu na lava
            if (isTileInArray(tilemap[selectedTileMapLine-1][selectedTileMapColumn-1], DANGEROUS_TILES, NUM_DANGEROUS_TILES))
                principal.isAlive = false;

            // Verifica moeda
            if (selectedTileMapColumn == COIN_COLUMN && selectedTileMapLine == COIN_LINE && !coin.isCollect)
            {
                coin.isCollect = true;
                std::cout << "Você coletou a moeda, vá para o tile preto!" << std::endl;
            }

            // Final tile
            if (tilemap[selectedTileMapLine-1][selectedTileMapColumn-1] == WALKABLE)
            {
                if (coin.isCollect) {
                    finalizarJogo();
                } else {
                    std::cout << "Você precisa coletar a moeda antes de chegar ao tile preto!" << std::endl;
                }
            }
            else {
                tilemap[selectedTileMapLine-1][selectedTileMapColumn-1] = WALKED;
            }
        }
        else
        {
            std::cout << "Movimento fora do mapa! (" << newLine << ", " << newCol << ")" << std::endl;
        }
    }
}


// Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO

int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
{

	ds = 1.0 / (float)nFrames;
	dt = 1.0 / (float)nAnimations;
	GLfloat vertices[] = {
		// x   y    z    s     t
		-0.5, 0.5, 0.0, 0.0, dt,   // V0
		-0.5, -0.5, 0.0, 0.0, 0.0, // V1
		0.5, 0.5, 0.0, ds, dt,	   // V2
		0.5, -0.5, 0.0, ds, 0.0	   // V3
	};

	GLuint VBO, VAO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// Ponteiro pro atributo 0 - Posição - coordenadas x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Ponteiro pro atributo 1 - Coordenada de textura s, t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

int setupTile(int nTiles, float &ds, float &dt)
{

    ds = 1.0 / (float)nTiles;
    dt = 1.0;

    // Como eu prefiro escalar depois, th e tw serão 1.0
    float th = 1.0, tw = 1.0;

    GLfloat vertices[] = {
        // x   y    z    s     t
        0.0, th / 2.0f, 0.0, 0.0, dt / 2.0f, // A
        tw / 2.0f, th, 0.0, ds / 2.0f, dt,   // B
        tw / 2.0f, 0.0, 0.0, ds / 2.0f, 0.0, // D
        tw, th / 2.0f, 0.0, ds, dt / 2.0f    // C
    };

    GLuint VBO, VAO;
    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);
    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);
    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);
    // Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
    //  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
    //  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
    //  Tipo do dado
    //  Se está normalizado (entre zero e um)
    //  Tamanho em bytes
    //  Deslocamento a partir do byte zero

    // Ponteiro pro atributo 0 - Posição - coordenadas x, y, z
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Ponteiro pro atributo 1 - Coordenada de textura s, t
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
    // atualmente vinculado - para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}

int loadTexture(string filePath, int &width, int &height)
{
    GLuint texID;

    // Gera o identificador da textura na memória
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int nrChannels;

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        if (nrChannels == 3) // jpg, bmp
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else // png
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

void desenharMapa(GLuint shaderID)
{
    // dá pra fazer um cálculo usando tilemap_width e tilemap_height
    float x0 = 575;
    float y0 = 100;

    for (int i = 0; i < TILEMAP_HEIGHT; i++)
    {
        for (int j = 0; j < TILEMAP_WIDTH; j++)
        {
            // Matriz de transformaçao do objeto - Matriz de modelo
            mat4 model = mat4(1); // matriz identidade

            Tile curr_tile = tileset[tilemap[i][j]];

            float x = x0 + (j - i) * curr_tile.dimensions.x / 2.0;
            float y = y0 + (j + i) * curr_tile.dimensions.y / 2.0;

            model = translate(model, vec3(x, y, 0.0));
            model = scale(model, curr_tile.dimensions);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

            vec2 offsetTex;

            offsetTex.s = curr_tile.iTile * curr_tile.ds;
            offsetTex.t = 0.0;
            glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

            glBindVertexArray(curr_tile.VAO);              // Conectando ao buffer de geometria
            glBindTexture(GL_TEXTURE_2D, curr_tile.texID); // Conectando ao buffer de textura

            // Chamada de desenho - drawcall
            // Poligono Preenchido - GL_TRIANGLES
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}

bool isTileInArray(TileType tileId, const TileType *tileArray, int arraySize) {
    for (int i = 0; i < arraySize; ++i) {
        if (tileArray[i] == tileId)
            return true;
    }
    return false;
}


void finalizarJogo()
{
    std::cout << "Você chegou ao final do jogo!" << std::endl;
    glfwTerminate();
    exit(0);
}