# Lords of Nightfall — Guia de Desenvolvimento & Contexto do Agente

> **Visão Geral**: RPG de ação 3D em terceira pessoa, mundo aberto para Linux e Windows.
> **Tech Stack**: C++20, CMake, Raylib (Gráficos/Math), Jolt Physics (Física 3D), Dear ImGui (Debug/Level Editor), Python (Asset Cooking).

---

## 1. Regras Críticas de Código & Convenções (Invioláveis)

### 1.1 Gerenciamento de Memória & STL
- **PROIBIDO usar Smart Pointers (`std::unique_ptr`, `std::shared_ptr`)**: O ciclo de vida e a posse (*ownership*) de objetos devem ser estritamente gerenciados por contêineres STL (como `std::vector`, `std::array`, `std::unordered_map`) ou por alocadores manuais centralizados.
- **Simplicidade & Direto ao Ponto**:
  - Prefira *structs* simples com variáveis públicas em vez de classes com *getters/setters* burocráticos.
  - Se uma função/lógica for usada em apenas um lugar e for pequena, mantenha-a *inline* ou local.
  - Antes de implementar qualquer funcionalidade, verifique se ela já existe para focar em reutilização.
  - Use bibliotecas padrão da STL (`std::string`, `std::vector`, `std::unordered_map`, etc.) sempre que aplicável.
  - Comente apenas o que não for imediatamente evidente pela leitura do código.

### 1.2 Nomenclatura & Formatação
| Elemento | Padrão | Exemplo |
| :--- | :--- | :--- |
| **Classes / Structs / Métodos / Funções** | `PascalCase` | `PlayerObject`, `UpdatePhysics()` |
| **Variáveis / Membros / Parâmetros** | `camelCase` | `playerHealth`, `moveSpeed` |
| **Namespaces** | `lowercase` (curto, 1 palavra ou abreviação) | `lon`, `core`, `gfx` |
| **Constantes / Macros** | `ALL_UPPER_CASE` | `MAX_ENEMIES`, `LON_ASSERT` |
| **Arquivos (Headers e Sources)** | `PascalCase` | `TerrainObject.h`, `EnemyAI.cpp` |
| **Header Guards** | `#ifndef` / `#define` | `#ifndef LON_SCENE_H` ... |

---

## 2. Arquitetura e Estrutura do Projeto

```
lords-of-nightfall/
├── CMakeLists.txt              # Script de build principal (Linux Nativo + MinGW Cross-Compile)
├── GEMINI.md / AGENTS.md        # Instruções de contexto e diretrizes para IA
├── Tools/                      # Ferramentas em Python (Cookers, Helpers)
│   ├── AssetCooker.py          # Entrypoint do Asset Cooker (Build Event)
│   └── cookers/                # Módulos especializados de conversão de assets
├── Game/
│   ├── Assets/                 # Raw assets brutos (FBX, PNG, WAV) — NUNCA carregados diretamente pelo engine
│   ├── ThirdParty/             # Dependências externas (raylib, jolt, imgui, etc.)
│   └── Source/                 # Código-fonte do jogo em C++
│       ├── Main.cpp            # Ponto de entrada (Main loop / Boot)
│       ├── App.h/.cpp          # Janela, input, active scene manager, gameloop
│       ├── Scene.h/.cpp        # Cenas, gerenciador de GameObjects, câmeras, UI
│       ├── GameObject.h        # Classe base abstrata de entidades 3D no mundo
│       ├── GameObjects/        # Especializações (Player, Enemy, Pickup, Terrain, etc.)
│       ├── Model/              # Leitor/Animador do formato binário próprio (.lonmodel)
│       ├── Components/         # Componentes de dados (Stats, Equipment, etc.)
│       └── ...                 # Sistemas de física, iluminação, inventory, settings, UI
└── Build/                      # Saída de compilação e artefatos
    ├── Debug/                  # Executável e assets para Debug
    ├── Development/            # Executável e assets para Development
    ├── Release/                # Executável e assets para Release final
    └── Intermediate/           # Arquivos temporários de compilação (Debug/Dev/Release)
```

---

## 3. Build System, Modos & Runtime

### 3.1 Plataformas & Compilação
- **Linux (Nativo)**: Compilado nativamente com GCC ou Clang.
- **Windows (Cross-Compilation)**: Compilado a partir do Linux usando `x86_64-w64-mingw32`.
  - As bibliotecas externas em `Game/ThirdParty/` (Raylib, Jolt, Dear ImGui) devem ser compiladas com a toolchain do MinGW para a build do Windows.

### 3.2 Modos de Build
| Modo | Símbolos de Debug | Otimizações | Ferramentas (Level Editor / ImGui) | Propósito |
| :--- | :---: | :---: | :---: | :--- |
| `Debug` | Sim | Baixa (-O0) | Sim | Desenvolvimento diário e depuração profunda |
| `Development` | Sim | Média/Alta (-O2) | Sim | Testes com performance otimizada mantendo ferramentas de editor |
| `Release` | Não | Máxima (-O3) | Não | Build final de entrega ao jogador (alta velocidade, sem ferramentas) |

### 3.3 Diretório de Runtime & Persistência
- Modificações temporárias de cena e dados de execução do Level Editor **DEVEM** ser salvos em `Build/<Config>/Saved/`.
- **NÃO** modificar diretamente a pasta `Game/Assets/` durante a execução do jogo para evitar conflitos de sincronização no CMake.

---

## 4. Pipeline de Assets & Asset Cooker

> **REGRA CRÍTICA DE ASSETS**: O jogo em tempo de execução **NUNCA** carrega arquivos `.fbx` brutos.
> O script `Tools/AssetCooker.py` é executado como **Build Event incremental** via CMake, processando apenas arquivos modificados (timestamp check) e convertendo-os em formatos binários proprietários (`.lonmodel`, etc.) copiados para a pasta `Build/<Config>/assets/`.

### 4.1 Requisitos de Ambiente Python
- Usar ambiente virtual `.venv` com dependências listadas em `requirements.txt` (incluindo `assimp`, etc.).

### 4.2 Assets de Personagens
- **Animações**: `Game/Assets/Universal Animation Library/` (`UAL1_standard.fbx`) e `Game/Assets/Universal Animation Library 2/` (`UAL2_standard.fbx`).
- **Personagem Base**: `Game/Assets/Universal Base Characters/superhero_male_fullbody.fbx` (modelo base rigado).
- **Outfits Modulares**: `Game/Assets/Modular Character Outfits/` (`Male_Ranger.fbx` e `Male_Peasant.fbx`).

### 4.3 Regras Críticas do Blender 4.4+ (Headless Animation Baking)
1. **Retargeting de Armadura**: O modificador `Armature` das submeshes deve ser explicitamente redirecionado para o esqueleto animado (`anim_armature`).
2. **Action Slots (Obrigatório)**: É mandatório atribuir o action slot ao carregar uma animação:
   ```python
   active_armature.animation_data.action = act
   active_armature.animation_data.action_slot = slots[0]
   ```
3. **View Layer Update**: Invocar sempre `bpy.context.view_layer.update()` antes de extrair a malha avaliada (`to_mesh()`) a cada quadro.
4. **Animação Idle Padrão (Obrigatório)**: A animação padrão do personagem parado **DEVE SEMPRE** ser em pé (`Idle_Loop`). **JAMAIS** utilizar `Crouch_Idle` ou qualquer animação agachada.

---

## 5. Roteiro de Milestones & Requisitos de Gameplay

### Milestone 01: Estrutura do Projeto
- Configuração do `CMakeLists.txt` (Linux nativo e cross-compile Windows via MinGW).
- Integração e linkagem das bibliotecas em `Game/ThirdParty/` (Raylib, Jolt, ImGui).
- Execução do `Tools/AssetCooker.py` como build event via CMake.

### Milestone 02: Base Jogável 3D
- Movimentação básica do player (WASD + Pulo com Espaço).
- Câmera em terceira pessoa controlável pelo mouse com **Camera Collision**.
- Sistema de terreno e sombras direcionais do sol.

### Milestone 03: Menus (Principal, Pausa e Configurações)
- **Menu Principal**: Cena própria com terreno, câmera orbital, título (*Lords of Nightfall*), botões Jogar, Configurações, Sair, e créditos no canto inferior esquerdo (*Stormhowl Studios* + Build version).
- **Menu de Pausa**: Acionado por `Esc` na cena de gameplay. Pausa o jogo, exibe overlay escuro com opções Continuar, Configurações, Voltar ao Menu e Sair.
- **Menu de Configurações Modular (Reutilizável)**:
  - *Gráficos*: Modo Fullscreen/Janela, Resolução de tela, Qualidade das sombras.
  - *Gameplay*: Sensibilidade do mouse look.
  - *Áudio*: Controle de volume de som.
- **UI Assets**: Interface visual com assets Kenney Fantasy UI (`Game/Assets/kenney_fantasy-ui-borders`, PNGs double resolution) processados via Asset Cooker.

### Milestone 04: Personagem Principal & Animações
- Substituição do cubo pelo modelo base rigado (`superhero_male_fullbody.fbx`).
- Execução fluida de animações (Idle em pé, Walk, Run, Jump, etc.) via leitor binário próprio.
- Suporte a variações modulares de skins (`Male_Ranger`, `Male_Peasant`).

### Milestone 05: Combate, Stats, Inventário & Equipamentos
- **Combate**: Ataques disparados ao clicar/segurar o botão esquerdo do mouse, com animações e feedback de dano.
- **Stats**: Barras de Vida e Experiência (XP), sistema de progressão/nível do personagem na UI.
- **Inventário & Itens**:
  - Spawning de itens coletáveis (`Ultimate RPG Items Pack`) no chão com prompt `[E] Coletar`.
  - UI de inventário com slots, suporte a drag & drop, dropar item, equipar armaduras/armas e consumir poções de vida.
  - Equipar armaduras altera a malha visual do personagem; equipar armas instancia/anexa a arma na mão.

### Milestone 06: Sistema de Inimigos & IA
- **Entidades de Monstros** (`Ultimate Monsters`: categorias *flying*, *blob*, *big*).
- **Parâmetros Dinâmicos**: Vida, Ataque, Defesa e Nível configuráveis por monstro.
- **Máquina de Estados de IA**:
  - *Patrol*: Fica parado por alguns segundos, caminha até um ponto aleatório no raio de patrulha ao redor do spawn point, repete.
  - *Chase*: Persegue o jogador se entrar no raio de detecção (a menos que o player seja 5+ níveis superior, onde o monstro permanece passivo até ser atacado).
  - *Attack*: Ataca e pausa em intervalos aleatórios baseados na agressividade da instância. Retorna a Patrol se o player morrer ou fugir.
- **UI Flutuante**: Nome, Nível e Barra de Vida sobre a cabeça do monstro exibidos quando o jogador está próximo ou se o monstro estiver com vida < 100%.
- **Recompensas**: Monstros derrotados dropam dinheiro e XP.

### Milestone 07: Cenário & Natureza
- Importação do kit de natureza (`Stylized Nature Mega Kit`).
- Spawning aleatório de vegetação no mapa; árvores possuem colisão física, demais props não possuem.

### Milestone 08: Level Editor
- Exclusivo para builds `Debug` e `Development`.
- Tecla de atalho `F1` para abrir/fechar; pausa a simulação do jogo e ativa a **Free Camera** (WASD + botão direito do mouse).
- **Sidebar Kenney Fantasy UI**: Interface nativa Raylib com fundo escuro, tabs douradas e scroll de listas.
- **Ferramentas**:
  1. *Terraformagem & Pintura*: 
     - **Elevação**: Brush para alterar geometria/elevação do relevo (clique para subir, Ctrl para descer, scroll para raio).
     - **Pintura de Textura (Stencil / Splatmap)**: Permite pintar livremente 4 texturas (`Grama`, `Terra`, `Areia`, `Pavimento`) usando um mapa de stencil RGBA com blend suave e shader customizado.
     - **Escala de UV**: Sliders dedicados para ajustar o tiling e mapeamento UV individual de cada textura (`uvScaleGrass`, `uvScaleDirt`, `uvScaleSand`, `uvScalePavement`).
     - **Regra de SRP & Sincronização de Altura (`TerrainAligner`)**: Durante a deformação do relevo, a classe dedicada `TerrainAligner` (`Game/Source/TerrainAligner.h/.cpp`) recalcula imediatamente a posição vertical (`position.y`) e os corpos físicos Jolt (árvores, CharacterVirtual do jogador, alvos de patrulha dos monstros, coletáveis) de todas as entidades dentro do raio do brush para evitar que atravessem o chão ou fiquem flutuando.
  2. *Props*: Paleta com listagem, preview sobre a superfície, clique para adicionar e Ctrl+clique para deletar.
  3. *Inimigos*: Lista com monstros e níveis, preview de posicionamento de spawns e botão para resetar posições de teste.
  4. *Coletáveis*: Adição e remoção de itens coletáveis no cenário.
- **Persistência (.lvl)**: Alterações de relevo, stencil/splatmap em Base64, escalas de UV e entidades serializadas e salvas automaticamente em `Game/Assets/Levels/Default.lvl` (e no diretório de runtime da build), processadas pelo Asset Cooker.

### Milestone 09: Minimap

### Milestone 10: Quests e NPCs

---

## 6. Lista de Polimentos e Correções Pendentes (Milestone XX)
- [ ] Ajustar cores do menu para um estilo mais flat e limpo.
- [ ] Corrigir clipping de geometria entre a armadura/roupa equipada e o corpo base do jogador.
- [ ] Corrigir o instanciamento e alinhamento do transform das armas equipadas na mão do personagem.