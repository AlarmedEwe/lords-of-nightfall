# Jogo de RPG Mundo Aberto

Nós vamos criar um jogo de RPG em terceira pessoa, em 3D, mundo aberto para computador, especificamente para sistema operacional Linux, onde o jogador vai conseguir explorar o mundo, matar inimigos de diferentes níveis, e cada inimigo vai ter níveis e, consequentemente, vida e defesa e ataques com variáveis diferentes.
O jogador vai poder encontrar ou coletar itens que são equipamentos e armas que ele pode melhorar.
Ele vai ter uma barra de experiência e uma barra de vida.
Ele vai poder encontrar poção de vida para recuperar a vida dele, ter um inventário, equipar armas, subir de nível, etc.
Ele também vai poder encontrar NPCs que ele vai conversar através de uma interface, recebendo quests e ir buscar um item em determinado lugar, ou então matar uma quantidade x de inimigos e essa quest vai dar uma recompensa para ele.

---

# Tecnologia e estrutura

## Ferramentas e plataformas
- Plataforma: Linux/Windows
    - O GCC compila pra Linux
    - MinGW-w64 realiza a cross-compilation
        - As bibliotecas externas (Raylib, Jolt, ImGui) precisam ser compiladas utilizando a toolchain do MinGW
- Linguagem de programação: **C++**
    - Compilador: GCC, CMake
    - Build modes:
        - Debug
        - Development
        - Release (para a build final)
- Bibliotecas externas (Third party):
    - Gráfica: raylib (https://github.com/raysan5/raylib)
    - Física: Jolt (https://github.com/jrouwe/joltphysics)
    - Interface de Debug: Dear ImGui (https://github.com/ocornut/imgui)
- Linguagem para os Tools: Python

## Estrutura do projeto
- Game/Assets/
- Game/Source/
- Game/ThirdParty/
- Tools/AssetCooker.py
- MakeFile
- Build/Debug/
- Build/Development/
- Build/Release/
- Build/Intermediate/Debug/
- Build/Intermediate/Development/
- Build/Intermediate/Release/

## Convenções de código
Nomenclatura:
- Use sempre **PascalCase** para nome de classes, structs, funções ou métodos.
- Sempre use **camelCase** para nomes de variáveis.
- Namespaces devem ser curtos, de uma única palavra (ou abreviação) e **all lower case**.
- Macros devem ser **ALL_UPPER_CASE**.
- Nome de arquivos devem ser sempre em **PascalCase**.
- Use include guards (ifndef define) para os headers.

Estilo de programação:
- Mantenha o código simples e direto ao ponto.
- Não crie funções só porque você acha que deve criar.
- Se algo pode ser feito inline e só será usado em um único lugar, deixe-o inline.
- Não faça variáveis privadas com getters e setters a não ser que tenha um motivo claro. Prefira estruturas simples com variáveis públicas.
- Antes de implementar um recurso, verifique se ele já existe. Foque em reusar o que já tem.
- Não precisa comentar tudo, apenas o que não é imediatamente claro apenas lendo o código.
- Você pode usar STL (std::string, std::vector, std::unordered_map, etc).
- Se algo é possível ser feito usando as bibliotecas padrões da linguagem, prefira fazer dessa forma.
- Não use smart pointers.

## Build Modes
- **Debug** e **Development** são para desenvolvimento, eles contêm símbolos para debug e também ferramentas extras de level design, etc.
- **Release** é para o jogo final, que será shipado. Não contém símbolos, foca em velocidade, não tem ferramentas de level design, etc. A diferença do Release e Development é basicamente que o development tem essas ferramentas extras.
- **Debug** pode ser mais lento por conta dos símbolos, etc. **Development** deve ter uma performance melhor.

---

# Tools (Ferramentas)

Nós teremos ferramentas para ajudar no desenvolvimento do jogo. Essas ferramentas podem ser escritas em Python.
E um exemplo de ferramenta é uma ferramenta de asset cooker, que vai basicamente pegar todos os assets, os modelos em 3D e texturas, que vão estar na pasta `Game/Assets`, e elas vão e vai transformar, por exemplo, o FBX em um formato próprio binário que o nosso motor consegue ler, transformar as imagens de PNG em um formato que o motor consegue ler e também transformar os sons em um formato que o motor consegue ler. **O asset cooker é um build event**.

Na versão final do jogo, ou seja, junto ao executável de cada build, seja de debug, seja development, seja de release, vai ter uma pasta assets e dentro dessa pasta vão ter arquivos binários que só nosso motor de jogos vai conseguir ler também.
É importante deixar claro que o modo de debug e o modo de development vão ter ferramentas de desenvolvimento, só que essas vão ser desenvolvidas dentro do próprio motor em C++.
E é importante especificar já aqui, de uma vez, que essas ferramentas, elas vão alterar a estrutura de arquivos do jogo. Esses arquivos, essas cenas, principalmente porque, por exemplo, o editor de level do jogo vai editar a cena, ele vai ser salvo num arquivo binário e esse arquivo ele pode ficar dentro da própria pasta do arquivo de source do jogo, ou seja, `Game/Assets`, e aí é copiado na hora de compilar, na hora de dar build para junto ao executável.

---

# Milestones

## Milestone 01: Estrutura do projeto
Criar um "Hello world" para esse projeto, que compila, tem todas as bibliotecas clonadas para `Game/ThirdParty/` e linkadas no game. O projeto compila, abre, mas não tem absolutamente nada. As tools (AssetCooker.py) executam no build event, mas também não fazem nada.