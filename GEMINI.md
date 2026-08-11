# Jogo de RPG Mundo Aberto

Nós vamos criar um jogo de RPG em terceira pessoa, em 3D, mundo aberto para computador, para sistemas operacionais Linux e Windows, onde o jogador vai conseguir explorar o mundo, matar inimigos de diferentes níveis, e cada inimigo vai ter níveis e, consequentemente, vida e defesa e ataques com variáveis diferentes.
O jogador vai poder encontrar ou coletar itens que são equipamentos e armas que ele pode melhorar.
Ele vai ter uma barra de experiência e uma barra de vida.
Ele vai poder encontrar poção de vida para recuperar a vida dele, ter um inventário, equipar armas, subir de nível, etc.
Ele também vai poder encontrar NPCs que ele vai conversar através de uma interface, recebendo quests e ir buscar um item em determinado lugar, ou então matar uma quantidade x de inimigos e essa quest vai dar uma recompensa para ele.

---

# Tecnologia e estrutura

## Ferramentas e plataformas
- Plataforma: Linux / Windows
    - O GCC/Clang compila nativamente para Linux
    - MinGW-w64 realiza a compilação cruzada (cross-compilation) para Windows a partir do Linux
        - As bibliotecas externas (Raylib, Jolt, ImGui) precisam ser compiladas utilizando a toolchain do MinGW para a build do Windows
- Linguagem de programação: **C++**
    - Sistema de Build: **CMake** (utilizando Make ou Ninja como gerador subjacente)
    - Build modes:
        - Debug
        - Development
        - Release (para a build final)
- Bibliotecas externas (Third party):
    - Gráfica e Matemática Nativa: raylib (https://github.com/raysan5/raylib)
    - Física: Jolt (https://github.com/jrouwe/joltphysics)
    - Interface de Debug: Dear ImGui (https://github.com/ocornut/imgui)
- Linguagem para os Tools: Python

## Estrutura do projeto
- Game/Assets/ (Ativos brutos: modelos FBX, imagens PNG, áudios WAV)
- Game/Source/
- Game/ThirdParty/
- Tools/AssetCooker.py
- CMakeLists.txt
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
- Use include guards (#ifndef / #define) para os headers.

Estilo de programação e Gerenciamento de Memória:
- Mantenha o código simples e direto ao ponto.
- Não crie funções só porque você acha que deve criar.
- Se algo pode ser feito inline e só será usado em um único lugar, deixe-o inline.
- Não faça variáveis privadas com getters e setters a não ser que tenha um motivo claro. Prefira estruturas simples com variáveis públicas.
- Antes de implementar um recurso, verifique se ele já existe. Foque em reusar o que já tem.
- Não precisa comentar tudo, apenas o que não é imediatamente claro apenas lendo o código.
- Você pode usar STL (std::string, std::vector, std::unordered_map, etc).
- Se algo é possível ser feito usando as bibliotecas padrões da linguagem, prefira fazer dessa forma.
- Não use smart pointers. O ciclo de vida e a posse dos objetos (ownership) devem ser gerenciados estritamente por contêineres padrão da STL (ex: `std::vector` ou `std::array` como proprietários primários) ou por alocadores manuais centralizados.

## Build Modes
- **Debug** e **Development** são para desenvolvimento, eles contêm símbolos para debug e também ferramentas extras de level design, etc.
- **Release** é para o jogo final, que será shipado. Não contém símbolos, foca em velocidade, não tem ferramentas de level design, etc. A diferença do Release e Development é basicamente que o development tem essas ferramentas extras.
- **Debug** pode ser mais lento por conta dos símbolos, etc. **Development** deve ter uma performance melhor.

---

# Estrutura do código

Descrição de todo o código dentro de `/Game/Source/`:
- **Main.cpp**: Ponto de inicio do programa;
- **App.h/cpp**: Será a classe principal do jogo, responsável por cuidar da janela, sistema de inputs, gerenciar a cena ativa e executar o gameloop do jogo;
- **Scene.h/cpp**: Classe para as Cenas do jogo. Cada cena contém informações da câmera que será usada, uma lista de GameObjects, interface do jogo, etc. É responsável por gerenciar esses dados e gerenciar a lógica de cada objeto.
- **GameObject.h/cpp**: Classe GameObject, cada um representa um objeto no mundo em 3D e pode ter formas, lógicas e físicas diferentes. Feito para ser uma classe base que é especializada depois em classes filho, dentro da pasta `/Game/Source/GameObjects/`. Exemplo: TerrainObject para terrenos, PlayerObject para o player, EnemyObject para inimigos, NPCObject para NPCs, PropObject para props do cenário, etc.

---

# Tools (Ferramentas)

Nós teremos ferramentas para ajudar no desenvolvimento do jogo. Essas ferramentas podem ser escritas em Python.
E um exemplo de ferramenta é uma ferramenta de asset cooker, que vai basicamente pegar todos os assets, os modelos em 3D e texturas, que vão estar na pasta `Game/Assets`, e vai transformar, por exemplo, o FBX em um formato próprio binário que o nosso motor consegue ler, transformar as imagens de PNG em um formato que o motor consegue ler e também transformar os sons em um formato que o motor consegue ler. **O asset cooker é um build event**.

O Asset Cooker deve implementar verificação de marca de tempo (timestamp check / incremental build), reprocessando apenas os arquivos modificados na pasta `Game/Assets` para otimizar os tempos de compilação.

Na versão final do jogo, ou seja, junto ao executável de cada build, seja de debug, seja development, seja de release, vai ter uma pasta assets e dentro dessa pasta vão ter arquivos binários que só nosso motor de jogos vai conseguir ler também.

É importante deixar claro que o modo de debug e o modo de development vão ter ferramentas de desenvolvimento, só que essas vão ser desenvolvidas dentro do próprio motor em C++.
As ferramentas do motor (como o editor de level) salvarão modificações de cena temporárias e dados de execução em um diretório de runtime dedicado (`Build/<Config>/Saved/`), evitando a modificação direta do diretório de código-fonte `Game/Assets/` durante a execução do jogo para evitar conflitos de sincronização no CMake.

---

# Milestones

## Milestone 01: Estrutura do projeto
Criar um "Hello world" para esse projeto através do `CMakeLists.txt`, que compila nativamente para Linux e via MinGW para Windows, possui todas as bibliotecas clonadas para `Game/ThirdParty/` e linkadas no game. O projeto compila, abre, mas não tem absolutamente nada. As tools (`AssetCooker.py`) executam no build event via CMake, mas também não fazem nada.

## Milestone 02: Jogo "Base"
Ter uma "base jogável", ou seja: um cubo andando com W, A, S, D, pulando com espaço, câmera em terceira pessoa controlável com o mouse, camera collision, um sistema de terreno e sombras do sol.

## Milestone 03: Menu inicial e de pausa
Crie uma cena diferente, para o menu principal do jogo. Essa cena pode ser só um terreno com um cubo no meio por enquanto, e uma câmera girando em torno do cubo, pra ficar legal. E ai precisa de ter menus. O título do jogo (Lords of Nightfall). Opção de sair do jogo, opção de jogar e opção de configurações do jogo. No canto inferior esquerdo da tela, você pode colocar os créditos, que é "Stormhowl Studios" e a build do jogo.

Crie também na cena principal do jogo um menu de pausa. Quando o jogador apertar Esc, o jogo pausa. Você coloca um overlay escuro na frente e com opções de continuar o jogo, configurações, voltar para o menu e por último sair do jogo, que vai fechar o jogo. Assim como no menu principal vai sair. O menu de configurações do menu de pausa e do menu principal, eu quero que você faça modular, basicamente para a gente conseguir reutilizar o mesmo menu de configurações nos dois, já que eles vão se tratar da mesma coisa. E as configurações que eu quero que você coloque são:

Configurações gráficas para ajustar, por exemplo, se eu quero o jogo em full screen ou não, a qualidade das sombras e talvez a resolução do jogo. Pode ser por padrão customizado, que é o tamanho da tela. Depois de gameplay, eu quero que você crie uma configuração para o mouse look, para a sensibilidade do mouse. E também já crie de uma vez, já antecipando, configurações do volume de á do jogo.

Para o estilo visual da UI do jogo, utilize os assets que eu coloquei na pasta `Game/Assets/kenney_fantasy-ui-borders`. E para a primeira vez, isso significa que você vai ter que usar o nosso sistema de asset cooker, ou seja, o arquivo Python dentro da pasta tools. Preciso que você faça ele pegar as imagens que você vai precisar lá no Kenney, especificamente as imagens em PNG com resolução double e copie elas para junto da pasta final do jogo, da build do jogo, na pasta assets, com o formato que o nosso motor consegue ler rapidamente e carregar na memória para você utilizar na interface.

## Milestone 04: Personagem principal com animações

## Milestone 05: Sistema de vida, XP do personagem + ataques, etc.

## Milestone 06: Inimigos (lógica + animações)

## Milestone XX (final): Polimentos e otimizações
- Mude as cores do menu para deixar mais flat