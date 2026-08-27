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
# Assets 3D do jogo
**MUITO IMPORTANTE:** Esses assets em formato FBX NÃO são para serem importados diretamente em FBX no jogo. Ao invés disso, use o script `Tools/AssetCooker.py` para extrair esses arquivos e escrever um arquivo final em binário junto ao `.exe`, num formato que o jogo consiga ler e importar de forma rápida e fácil. Use assimp para importar os FBX. Crie um `.venv` do Python e um `requirements.txt` para instalar essas coisas.

## Assets dos personagens
- Nas pastas `Game/Assets/Universal Animation Library/` e `Game/Assets/Universal Animation Library 2/`, você vai encontrar FBXs contendo inúmeras animações: `UAL1_standard.fbx` e `UAL2_standard.fbx`.
- Em `Game/Assets/Universal Base Characters/`, você vai encontrar uma personagem base já rigado (`superhero_male_fullbody.fbx`) para ser o personagem principal do jogo. Essa skin é o personagem "só de cueca", sem roupas.
- Em `Game/Assets/Modular Character Outfits/`, você vai encontrar 4 modelos diferentes de outfits, sendo que vamos usar dois: `Male_Ranger.fbx` e `Male_Peasant.fbx`. Ambos já estão rigados e vamos usar as partes individuais aqui para fazer os equipamentos.

### REGRA CRÍTICA DE ASSET COOKING DE ANIMAÇÕES (BLENDER 4.4+):
- O script `Tools/AssetCooker.py` gera e executa um script headless no Blender para assar as animações FBX dos personagens (`Superhero_Male`, `Male_Peasant`, `Male_Ranger`).
- **Retargeting de Armadura**: O modificador `Armature` das submeshes deve ser explicitamente redirecionado para o esqueleto animado importado (`anim_armature`).
- **Action Slots no Blender 4.4+**: É **obrigatório** atribuir o `action_slot` (`active_armature.animation_data.action_slot = slots[0]`) ao carregar uma ação (`animation_data.action = act`), caso contrário a animação será ignorada e o modelo ficará estático em T-Pose.
- **View Layer Update**: Deve-se chamar `bpy.context.view_layer.update()` antes de extrair a malha avaliada (`to_mesh()`) a cada quadro.

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
Agora nós vamos melhorar o nosso sistema de asset cooking para ler o FBX e criar um formato próprio junto ao executável do jogo dentro da pasta assets, que vai junto ao executável de forma que a gente consiga importar os nossos modelos e animações e executar eles dentro do jogo. Implemente a seção Assets 3D do jogo neste documento. Especifique os assets do personagem. O personagem principal, então, deve ter a forma visual do personagem base e deve ter animações: ele parado, ele andando, correndo, pulando, etc.
**REGRA OBRIGATÓRIA DE ANIMAÇÃO:** A animação base do personagem parado DEVE ser SEMPRE a animação padrão em pé (`Idle_Loop`), JAMAIS utilizar `Crouch_Idle` ou qualquer outra animação agachada.

## Milestone 05: Sistema de vida, XP do personagem + ataques, etc.
Adicionei vários itens em FBX em `Game/Assets/Ultimate RPG Items Pack`. Itens como armas por personagem, machado, espada, etc, poções. Agora quero que você implemente ataques para quando eu apertar o botão esquerdo, clicar e segurar o botão esquerdo do mouse, o personagem conseguir atacar com animação e também tenha o sistema de vida, de experiência do personagem, etc. Então, já vai fazer a interface do jogo aqui, bem bonito, bem polida, com tudo que precisa para esse jogo.

E aí eu quero que você coloque no chão ali, espalhado para gente testar itens que, se eu chegar perto, vai aparecer na interface para eu apertar `E` e coletar. E se eu apertar E e coletar, ele coleta para o inventário. Então, eu preciso também ter um sistema de inventário que eu possa abrir e fechar na interface, e aí esse inventário vai ter os slots, e eu posso arrastar os slots de um lugar para o outro, dropar o item ou equipar o item, ou também utilizar o item no caso da poção. Eu também quero itens no chão com armadura para eu coletar, equipar e isso alterar o visual do personagem para os dois FBXs que eu tenho na pasta para as skins do personagem. Lembre de usar o sistema de asset cooking.

## Milestone 06: Inimigos (lógica + animações)
Agora nós vamos implementar os inimigos. Eles precisam ser um game object específico. Não acho que vale a pena criar um game object só para todos os inimigos, porque você vai ver que são dezenas deles, mas um game object que a gente consiga configurar ali para ser, para selecionar inimigos diferentes.

Isso porque dentro da pasta de assets, você vai encontrar uma pasta chamada `Ultimate Monsters`. E dentro dessa pasta, você vai ter três categorias de inimigos: flying, blob e big. Todos eles vão ter a mesma mecânica. Eles vão andar, só muda as animações e o modelo 3D e texturas. E obviamente, eu quero que você mude o nível deles: vida, ataque, etc. Cada uma dessas pastas contém vários FBXs, um para cada inimigo. E cada inimigo tem várias animações. Essas animações são: Death, Duck, HitReact, Idle, Jump, Jump_Idle, Jump_Land, No, Punch, Run, Walk, Wave, Weapon, Yes.

A lógica do inimigo é a seguinte: eles são spawnados e eles vão armazenar a posição que eles são spawnados inicialmente e eles vão ter uma lógica padrão, um estado de patrulha, que eles vão ficar parados por alguns segundos, depois vão andar até uma direção específica até eles chegarem naquela posição. Depois voltam a ficar parados por um segundo e repete. Essa direção deve estar sempre dentro do raio próximo do lugar de spawn deles e aí você pode definir um raio de patrulha para eles ficarem. Se o personagem chegar perto deles, eles imediatamente mudam para o estado de ir atrás do player e aí eles vão correr ao invés de andar e em direção ao player. Chegando perto do player, eles simplesmente ficam parados, atacam, depois ficam parados de novo, atacam e isso aleatoriamente. Eles ficam parados por algum tempo aleatório dependendo se e aí cada inimigo pode ter uma chance percentual de ser agressivo. E aí isso é aí é aleatório por inimigo, por instância de inimigo. E se ele for agressivo, ele basicamente vai ficar menos tempo parado e vai atacar mais frequentemente. Bem simples a lógica. Se o player morrer, eles voltam para o estado de patrulha. Outra coisa, se o player for cinco níveis ou mais acima dos inimigos, eles não vão atacar o player por padrão. Só atacam, só vão continuar no estado de patrulha, porque aí eles vão entender que o personagem é muito forte, mas obviamente se o personagem atacar eles primeiro, aí sim eles atacam.

Você vai exibir o nome do inimigo, o nível e a barra de vida em cima da cabeça deles. Isso será exibido se uma de duas condições for verdadeira: primeiro, se o jogador estiver próximo do inimigo, porque aí o jogador consegue saber o nível e o nome de cada inimigo, ou se a vida do inimigo não estiver em 100%, ou seja, se o inimigo tomou algum dano. Aí sim você pode exibir sempre na interface a vida dele.

Você vai implementar no sistema, claro, de ataque e de morte, tanto no player quanto no inimigo. Quando o inimigo morre, ele dropa dinheiro e experiência. Para testarmos, popule a cena de gameplay com centenas de inimigos aleatórios.

## Milestone 07: Cenário
Importe todos os packs de natureza da pasta `Stylized Nature MegaKit` para o jogo. Para fins de testes, spawne aleatoriamente eles pelo mapa. Árvores precisam de colisão, o resto não.

## Milestone 08: Level Editor
Agora, no modo debug development da build, nós vamos ter um level editor. A qualquer momento, durante a gameplay, o jogador, que no caso vai ser o desenvolvedor, vai poder apertar um botão para abrir ou ocultar o level editor. A gente vai usar o Dear ImGui para fazer isso. Quando aparecer esse editor, vai funcionar mais ou menos o seguinte: a gente tem algumas ferramentas. A primeira ferramenta é a ferramenta de terraformagem. Ah, e quando ele abre o editor de level, o mapa vai, o jogo vai ser pausado, ou seja, os inimigos, o personagem, etc., vai parar de ter os controles e a câmera vai entrar em no modo de câmera livre, onde, se o personagem clicar e segurar o botão direito, ele consegue controlar a câmera com o mouse look e voar livremente com WASD, como se fosse em um motor de jogos, que aí ele vai conseguir posicionar ela onde ele quiser. A primeira ferramenta do editor de level vai ser a ferramenta de terraformagem, que ele vai ter um brush para mudar a escala, a geometria do terreno. Então, ele pode clicar para subir o terreno, segurar Ctrl para deletar o terreno e por aí vai. Ele pode usar a rodinha do mouse para aumentar ou diminuir o tamanho do brush. Pronto. A segunda ferramenta vai ser a ferramenta de adicionar as props do cenário, que a gente importou da na milestone anterior. Quando o personagem clicar, vai ter uma lista com a thumbnail, contendo uma imagem que você vai renderizar para cada uma das props, porque aí o personagem pode simplesmente clicar e selecionar a que ele quiser. E aí, quando ele colocar o mouse em cima da superfície, vai aparecer uma preview dessa prop e ele pode clicar para adicionar. Ele também pode segurar Ctrl e clicar numa prop já adicionada para deletar essa prop. A terceira ferramenta é para adicionar inimigos, que vai funcionar exatamente igual à ferramenta anterior, a diferença é que vai ter uma lista de inimigos, não uma de props, também com as thumbnails e imagens. E aí, no caso da lista de inimigos, é importante ter o ícone de cada inimigo exibindo aí. E a última ferramenta é exatamente igual às anteriores, só que de itens coletáveis que o jogador pode adicionar. Toda vez que o jogador, que o desenvolvedor sair do modo de edição, isso deve ser salvo nos assets do jogo, e aí isso vai persistir, obviamente, se o desenvolvedor compilar o jogo de novo ou compilar num modo diferente. Essas mudanças vão persistir que ele fez, as adições e coisas que ele deletou. Uma coisa importante é que, quando o stage level estiver ativo, é claro que, como o jogo vai estar rodando, os inimigos vão ter mudado de posição e tudo mais. Então, é por isso que você, que no stage level você vai colocar no próprio level alguns ícones demarcando o spawn dos inimigos. E vai ter um botão lá para resetar todos os inimigos, todas as posições, porque aí, obviamente, você vai salvar o spawn dos inimigos e qual o inimigo é na posição original, porque isso vai mudar.

## Milestone XX (final): Polimentos e otimizações
- Mude as cores do menu para deixar mais flat
- A roupa do personagem equipado clipa com a malha do player.
- O instanciamento das armas equipadas tá errado.