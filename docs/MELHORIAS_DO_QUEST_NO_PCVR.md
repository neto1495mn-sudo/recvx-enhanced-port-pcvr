# Melhorias vindas do port para Quest, aplicadas ao PCVR

Documento para quem for escrever as notas de versão (ou para uma IA de release): o que aconteceu, por que, o que foi
mudado em cada arquivo, como foi medido e o que **não** foi verificado.

Branch: `melhorias-quest-controle` (a partir da `vr`, versão 1.4.1). Três commits de código, mais este documento.

## Contexto

O mesmo código foi levado para um Quest 2 rodando nativo (Android, 32 bits). Lá, jogando de verdade, apareceram
problemas de fluidez e de controle que também existem no PC. As correções que valem para o PC foram portadas para cá.
As que são específicas do Android (GPU do Quest, 90 Hz do visor, formato dos alvos de cor, carregamento) **não** foram
levadas.

Tudo que está abaixo marcado como "medido" tem número de log. O que é hipótese ou não foi testado está dito.

## O que foi corrigido

### 1. O personagem parava de andar no meio da caminhada (analógico e gatilho "caindo")

- **Sintoma:** andando em linha reta, o personagem dava uma paradinha e o som dos passos cortava; a chuva (outro som)
  continuava.
- **Medido (Quest 2, leitura bruta a 90 Hz):** em 55 s, 16 quedas do analógico esquerdo "para o mesmo lado". O valor cai
  de ~1,0 para 0,0–0,3 **numa única leitura** (11 ms), o que um polegar não faz, e volta em 10–110 ms (mediana 12 ms,
  90% abaixo de 45 ms, máximo 57 ms). O jogo recebia 0 por 1 a 4 quadros, saía do estado de andar (modo 6 para 0) e
  os passos paravam. Não foi medido em controle no PC via Virtual Desktop.
- **Correção (`pc/src/host/xr.c`, `axis_hold`):** depois de um controle ficar firme (> 0,7) por 150 ms, uma queda de mais
  de 0,4 numa leitura mantém o último valor até voltar, por no máximo 130 ms; uma queda mais lenta (2–3 leituras) é
  coberta por 50 ms. Vale para o analógico esquerdo, o direito (giro) e o gatilho esquerdo (corrida). **Fixo no código,
  sem configuração.** Custo: soltar o analógico de verdade passa a ser percebido até 130 ms depois.

### 2. A corrida (clique do analógico esquerdo) caía sozinha

- **Causa:** `sprint_update` cancelava a corrida no primeiro instante em que o analógico não apontava para a frente, o
  que acontecia nas quedas acima. O personagem passava a andar devagar (0,04 m por passo contra 0,105) até o próximo
  clique.
- **Correção (`pc/glue/vr_glue.c`):** a corrida só cai depois de 0,5 s realmente parado (analógico sem ir para a frente
  ou empurrando sem sair do lugar, como numa parede). Depois disso é preciso clicar de novo.

### 3. A câmera era jogada para a frente e voltava ao subir escadas

- **Medido (registro por quadro do mundo/câmera):** nos quadros logo depois do fim da escada a base da câmera ia de
  11,504 para 12,032 (+0,53 m) e depois para 10,321 (−1,71 m), voltando em ~15 quadros (decaimento ×0,75), com o
  personagem parado em 11,51. Só acontecia **subindo**.
- **Causa (a conta reproduz os números medidos):** `play_base` calculava o alvo da câmera com a âncora do modo estável
  (`steady_anchor`) que não é atualizada durante a escada (a câmera segue a raiz do esqueleto). No primeiro quadro depois
  da escada o alvo saía da âncora velha (~2 m atrás), a base suavizava 25% até lá e, no quadro seguinte, a âncora já era
  reiniciada mas a diferença guardada continuava valendo.
- **Correção (`pc/glue/vr_glue.c`):** regra geral `anchor_live`: a âncora só vale enquanto está em uso; sempre que a
  câmera foi calculada de outro jeito (escada, sem modelo, ou `play_base` sem rodar por mais de 300 ms) ela é reiniciada
  **antes** de calcular o alvo. A suavização entre modos só vale para diferenças menores que 0,8 m (antes 3 m).
- Também: a câmera nas escadas segue a raiz com velocidade prevista (a raiz sobe com um tranco de 0,2 m num quadro no
  começo de cada ciclo da animação).
- Um detector permanente (`vr: CAMERA JUMP ... m in one game frame`, com `vr_debug_log`) registra qualquer salto de
  câmera acima de 0,3 m num quadro, com o estado em volta.

### 4. Outras correções em `vr_glue.c`

- **Personagem girando sozinho** ao voltar do menu Start ou ao recentrar: o reset do rumo contava o giro da cabeça duas
  vezes. Agora `vr_yaw = facing - giro_da_cabeça`.
- **Recarga que subia uma munição só:** `bhSearchBullet` do jogo junta uma pilha de munição por chamada; a recarga por
  gesto agora repete (até 8 passagens) até a arma encher ou a munição acabar. **Não foi confirmado em jogo.**
- Diagnósticos leves (com `vr_debug_log`): "walk summary" por 10 s e "WALKING ON THE SPOT" (já existia).

### 5. Teleporte para sala que não existe travava o jogo na tela preta

- **Causa:** o teleporte dos Extras monta o arquivo da sala a partir de fase + sala (`RM_<fase><sala 2 dígitos><variante>`).
  Fase 0 sala 20 e sala 22 (`RM_0200`, `RM_0220`) não existem na tabela do disco (205 salas; na fase 0 vão de 0 a 16).
- **Correção (`pc/glue/cheat_glue.c`, `pc/src/audio/adx.c`):** o teleporte confere a tabela do disco e a pasta de mods
  (`pc_mod_exists`) e responde `no such room (RM_xxxx)` em vez de teleportar. Testado contra as 1000 combinações de
  fase/sala: 177 válidas aceitas, 0 divergências. A posição (0–31) continua sem validação.

### 6. Tempo do mundo nas imagens intermediárias (`gs_gl.c`, `render.c`)

- O tempo `t` de cada imagem entre dois quadros do jogo passou a avançar pela distância exata entre os horários de
  exibição do visor (`xr_display_time_ns`), puxado de leve pela chegada dos quadros (`g_vr_frame_seq`), em vez de contar
  imagens. Medido no Quest: imagens "paradas" e "pulando" caíram de ~25 por segundo cada para perto de 0.
- O par pode passar um pouco de 0 e de 1 (`RECVX_VR_MARGIN` 0,3, `RECVX_VR_BIAS` 0,25): os quadros chegam alguns ms antes
  ou depois do relógio. **Atenção:** é extrapolação limitada; nas telas em que a animação muda de ritmo ela pode
  passar um pouco da posição certa. Se aparecer um elástico visível no movimento, `RECVX_VR_MARGIN=0` e `RECVX_VR_BIAS=0`
  voltam ao relógio anterior.
- O andar mostrado usa uma velocidade suavizada (`walk_smooth`, `RECVX_WALK_SMOOTH=0` desliga): o passo do corpo por
  quadro é irregular (0, depois o dobro, passos curtos) e mostrar cada passo por si só dava um "puxão" 30 vezes por segundo.

### 7. Sala da chuva (RM_3210): a interpolação e os lotes de desenho (o maior ganho no PC)

Medido **em VR no PC** (Quest 2 por Virtual Desktop), carregando o save do dono em RM_3210 e olhando para a chuva:

| | antes | depois |
|---|---|---|
| imagens para o visor por segundo | 27–30 | **90** |
| desenhos por imagem | 5706 | ~280 |
| imagem 3D (média, `vr-diag: timing`) | ~10 ms (orçamento 11,1; todas lentas) | 1,8 ms (nenhuma lenta) |
| lotes encerrados só porque o "epoch" da textura mudou | 79 248 por segundo | 0 |

- **Causa:** `batch_for` encerrava o lote em quase toda primitiva porque o epoch de textura avançava entre elas (uma
  textura/CLUT carregada no meio), embora o resto do estado fosse idêntico.
- **Correção (`pc/src/gs/gs_gl.c`, `batch_for`):** na reprodução (`gs_replaying`), se só o epoch difere e a textura do lote
  resolve para a mesma nos dois epochs (e não há recarga de alvo entre eles), o lote continua. Mesmo bloco que já existia
  no port do Quest.
- **`pc/src/gs/gs.c`, `flush_for`:** em VR as escritas de registrador não encerram mais o lote; quem encerra é o
  `batch_for` quando o estado da próxima primitiva muda (`gs_lazy_flush` em `gs_gl.c`; `RECVX_NO_LAZY_FLUSH=1` volta ao
  antigo). Sozinho isso não mudou os desenhos; o ganho veio do ponto anterior.
- **Casamento da interpolação (`pc/src/gs/stream.c`, `stream_match`):** a sala tem ~1000 instâncias idênticas (os pingos)
  e cada uma era pontuada contra todas as do quadro anterior (~530 mil chamadas de `inst_pair` por quadro do jogo):
  7,8 ms em média (até 12) no PC e 17 ms no Quest 2. Agora os candidatos ficam ordenados por (malha, nº de grupos,
  índice) e o par é procurado primeiro entre os vizinhos do ponto onde o último par parou, aceitando na hora um a menos
  de 8 px; só se nenhum estiver perto é que há varredura completa (limitada a ±96 vizinhos e abandonada depois de 6
  falhas seguidas, para multidões de mais de 200 instâncias). Medido no PC com o save do dono: 0,4–0,8 ms por quadro;
  vértices pareados 99,9% nos dois jeitos (mínimo 90% contra 72%).
- **Registro por segundo (`render.c`, com `vr_debug_log`):** `pc: last second ...` (imagens, quadros do jogo, tempo de
  execução e de casamento, desenhos e primitivas por quadro e por imagem) e `pc: draws ended ...` (por que cada desenho
  terminou). Foi o que apontou a causa e serve para o próximo gargalo.

## Arquivos alterados

`pc/glue/vr_glue.c`, `pc/glue/cheat_glue.c`, `pc/src/audio/adx.c`, `pc/src/host/xr.c`, `pc/src/host/render.c`,
`pc/src/gs/gs_gl.c`, `pc/src/gs/gs.c`, `pc/src/gs/stream.c`.

## Ficou de fora (é do Quest, não se aplica ou não foi portado)

- Ajustes de GPU do Quest: alvos de cor RGBA8 nos alvos de 2 camadas, foveação, 90 Hz pedido ao visor, nível de
  desempenho do headset, espera pelas imagens do olho sem bloquear a thread.
- Ritmo do carregamento (`NowLoadDisp` solto, que no Quest rodava a ~420 quadros por segundo numa sala inexistente).
- **Não portado, mas pode valer no PC:** não desenhar os quadros do jogo que o visor redesenha (no Quest isso economizou
  ~2 ms de CPU e ~3,6 ms de GPU por quadro do jogo). Hoje cada quadro do jogo ainda executa ~2900 desenhos (~9 ms) na sala
  da chuva. Cabe, mas deixa pouca folga; é o próximo passo se houver queda.
- A ferramenta de teste de salvar/carregar o save de teste no Quest (só para testes) não foi levada.

## O que não foi verificado

- Em VR no PC só foi medido o desempenho (imagens por segundo, desenhos). **Não** foi comparada a aparência por imagem:
  juntar lotes pode deixar a borda macia de uma gota se misturar com a de outra. No Quest foi comparada (olho esquerdo,
  antes e depois, só os pingos de chuva diferiam).
- A proteção do analógico foi medida num Quest 2 (nativo). No PC via Virtual Desktop o controle pode se comportar
  diferente; o log `vr: left stick hold: N drops held` mostra se ela atua.
- As correções de escada, de corrida e do rumo foram confirmadas pelo dono **no Quest**; no PC ainda não.
- Recarga: não confirmada em jogo.

## Como verificar rapidamente no PC

1. `recvx.ini` com `vr_debug_log = 1`; rodar `recvx.exe --vr --log`.
2. Na sala RM_3210 olhando para a chuva, procurar no `recvx.log` `pc: last second`: esperado ~90 imagens e
   ~280 desenhos por imagem. `RECVX_NO_LAZY_FLUSH=1` e as variáveis `RECVX_VR_*` acima permitem comparar.
3. Subir uma escada: não deve aparecer `CAMERA JUMP` no fim dela.
