# Projeto Tilemap Isométrico – Processamento Gráfico
**Disciplina:** Processamento Gráfico\
**Curso:** Ciência da Computação – Universidade do Vale do Rio dos Sinos, Polo Porto Alegre

## Descrição

O sistema lê mapas externos, permite fácil edição dos cenários, e usa OpenGL moderno para renderização e movimentação.\
Personagem e moeda são representados por sprites, e o desafio é coletar a moeda antes de chegar ao tile final. Obstáculos perigosos (ex: lava) e paredes são definidos pelo próprio mapa.

## Autores

- Arthur Kist Juchem
- Felipe Eduardo Mossmann
- Gustavo de Oliveira

## Requisitos

- **C++17** ou superior
- **CMake 3.10+**
- **OpenGL 3.3+**
- **GLFW** (janela/input)
- **GLEW** (extensões OpenGL)
- **GLM** (matemática gráfica)
- **stb\_image** (carregamento de texturas)
- **GLAD** (código já incluso no projeto)

## Estrutura do Mapa

O arquivo `Map.txt` deve seguir o padrão:

```
tilesetIso.png 7 75 45
15 15
@11113211111111
111113331113311
110013331111111
110113331111111
100111111111113
101111444441333
101111455541113
101111455541133
101111444441111
101111111110001
100000000000101
111111111101101
111111111101101
111111111103331
11111111110333C
nao-caminhaveis
45
perigosos
3
final
2pop
caminhado
6
coin.png 35 35
```


**Significado dos campos:**
- **Primeira linha:** nome do tileset, número total de tiles, largura e altura do tile (em pixels).
- **Segunda linha:** tamanho do mapa (linhas colunas).
- **Linhas seguintes:** desenho do mapa usando símbolos ou números para cada tile.
- **Palavras-chave:** marcam seções para indicar tipos de tiles:
    - `nao-caminhaveis` (lista de símbolos/valores para tiles bloqueados)
    - `perigosos` (lista de símbolos para tiles perigosos)
    - `final` (tiles que representam objetivo)
    - `caminhado` (tiles já percorridos pelo personagem)
- **Por fim:** arquivo do sprite da moeda e dimensões.

> **Dica:**  
> Os valores informados após cada palavra-chave podem ser um ou mais símbolos, números ou letras, sem espaço entre eles (ex: `45` para tiles 4 e 5).

## Controles

- **W, A, S, D, Q, E, Z, C:** Movimentam o personagem nas direções do tilemap isométrico
- **Objetivo:** Coletar a moeda (`C`) e chegar ao tile final
- **Atenção:** Não pise nos tiles perigosos (`3`)


## Como compilar e executar em diferentes sistemas operacionais

### **Linux (Ubuntu/Debian/Mint)**

1. Instale as dependências:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev libgl1-mesa-dev
   ```
2. Baixe/clique o projeto, acesse a pasta:
   ```bash
   cd pasta-do-projeto
   mkdir build
   cd build
   cmake ..
   make
   ```
3. Execute:
   ```bash
   ./FinalTaskGB
   ```

---

### **MacOS**

1. Instale o Homebrew, depois as dependências:
   ```bash
   brew install cmake glfw glew glm
   ```
2. Compile normalmente:
   ```bash
   cd pasta-do-projeto
   mkdir build
   cd build
   cmake ..
   make
   ```
3. Execute:
   ```bash
   ./FinalTaskGB
   ```

---

### **Windows**

1. Baixe e instale:
   - [Visual Studio Community](https://visualstudio.microsoft.com/pt-br/vs/)
   - [CMake](https://cmake.org/download/)
   - [MSYS2](https://www.msys2.org/) (opcional para facilitar dependências)
2. Instale as bibliotecas via vcpkg ou MSYS2:
   - **Com vcpkg:**
     ```bash
     git clone https://github.com/microsoft/vcpkg.git
     .\vcpkg\bootstrap-vcpkg.bat
     .\vcpkg\vcpkg install glfw3 glew glm
     ```
3. Gere a solução:
   - Com o CMake GUI, aponte para a pasta do projeto e gere para Visual Studio.
   - Compile e execute pelo Visual Studio.
   - Ou, no terminal:
     ```bash
     mkdir build
     cd build
     cmake .. -G "Visual Studio 17 2022"   # Ou sua versão instalada
     cmake --build . --config Release
     ```
   - O executável estará na subpasta `Release\FinalTaskGB.exe` ou similar.

---

## Observações finais

- Certifique-se de manter `Mapa.txt` na mesma pasta do executável ou ajuste o caminho no código.
- Caso altere o mapa, mantenha o padrão do arquivo exemplo.
- O projeto é acadêmico, uso livre para fins didáticos.
