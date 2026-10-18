# Mapa de mudanças do VR

Este arquivo conta **o que cada versão do VR mudou, por quê, e se ainda está valendo**.
Serve para, quando aparecer um bug, achar rápido o que pode ter causado.

Como ler a situação de cada item:

- ✅ **ATIVO** – roda sempre.
- 🔘 **SÓ COM OPÇÃO** – só roda com uma opção do `recvx.ini` ligada (diz qual).
- ❌ **DESFEITO** – foi tirado; diz em qual versão e por quê.
- 🔍 **SÓ LOG** – não muda o jogo, só escreve no `recvx.log` (com `vr_debug_log = 1`).
- ⚠️ **ATENÇÃO** – ativo, mas foi feito por causa de outra coisa; pode causar bug no futuro.

As versões antes da vr64 (vr1 a vr63) não têm registro aqui: foram a base do VR (3D, mãos,
armas, telão, recarga, dano por parte). As versões com "sem registro detalhado" foram
reconstruídas pelas etiquetas no código e pela conversa; o que não deu para confirmar está dito.

---

## 0. Fila da próxima versão (vr120)

O que o dono pediu para a próxima revisão, na ordem em que pediu. Ao começar a versão, cada item
vira plano → "pode" → feito, e sai daqui para a tabela da seção 2.

1. *(vazia – o dono vai mandar os itens)*

---

## 1. Mapa por assunto (o "mapa mental")

Cada linha é uma corrente: **problema → o que foi feito → situação**. Quando um bug aparecer,
procure o assunto e siga a corrente.

### Câmera e movimento

```
Andar balançava / enjoava (o corpo da personagem sacode ao andar)
 ├─ vr71  andar pelo analógico suavizado no ritmo do óculos ........ ✅ ATIVO (vr_interp_body = 1)
 ├─ vr72  só suaviza movimento contínuo (parede empurrando = sem suavizar) ✅ ATIVO
 ├─ vr73  posição usada é a do corpo, não a raiz que balança ........ ✅ ATIVO
 └─ vr75  CÂMERA ESTÁVEL (ignora os pequenos movimentos do corpo) ... 🔘 SÓ COM vr_cam_steady = 1 (hoje 0)
      ├─ vr76  a câmera segue só o andar do analógico/da sala ....... 🔘 vr_cam_steady
      ├─ vr77  catraca: só para frente; parede empurrando é ignorada . 🔘 vr_cam_steady
      ├─ vr79  câmera nunca à frente dela ............................ 🔘 vr_cam_steady
      ├─ vr80  câmera para na parede do jogo ......................... 🔘 vr_cam_steady
      ├─ vr88  depois da faca/mira, ela volta para baixo da câmera ... 🔘 vr_cam_steady
      ├─ vr91  câmera recomeça onde ela está depois da escada ........ 🔘 vr_cam_steady
      ├─ vr92–94 BUG "andando no mesmo lugar" (log para achar a causa)
      │     └─ causa (vr97): a catraca achava que andar para trás era parede empurrando
      ├─ vr97  "para trás" medido pela direção atual do analógico;
      │        se ficou para trás >0,3 m, alcança no passo dela ........ 🔘 vr_cam_steady
      ├─ vr98  câmera seguindo exata ................................. ❌ DESFEITO na vr99 (trancos mais fortes)
      └─ BUG que fez você desligar: a visão virava para trás / câmera puxava
            └─ causa provável: heading_update volta a visão para a frente dela quando
               o controle some ou ao recentrar. NÃO CONSERTADO (opção desligada).

Personagem fazia a volta para andar para trás (giro do jogo)
 └─ vr85  VIRAR NA HORA para o lado do analógico, sem arco ......... 🔘 SÓ COM vr_move_direct = 1 (hoje 0)
      └─ causou a visão virando e a câmera puxando junto com a câmera estável;
         você desligou as duas e "ficou bem melhor". Hoje ela gira como no jogo original.

vr84  andar de lado (strafe) ......................................... ❌ DESFEITO (quebrou o jogo: passo 2–4x maior;
                                                                         código voltou exatamente para a vr81)

Andar no quarto (room-scale)
 └─ vr80  andar no quarto para na parede do jogo (checagem de parede do próprio jogo) ✅ ATIVO
```

### Mira e andar mirando

```
Andar mirando (vr_aim_walk = 1)
 ├─ vr76  o andar mirando é nosso (o jogo não anda mirando) ......... ✅ ATIVO
 ├─ vr78  o passo mirando passa pela checagem de parede do jogo ..... ✅ ATIVO
 ├─ vr79  soltar a mira andando: anda na hora, sem a animação de abaixar a arma ✅ ATIVO
 ├─ vr95  BUG: o jogo puxava ela de volta enquanto mirava
 │        → os movimentos do próprio jogo durante a mira são desfeitos ⚠️ ATENÇÃO (ver seção 3)
 └─ vr96  velocidade 80% só mirando (65/66/67/69); levantar/abaixar arma e faca = 100% ✅ ATIVO
```

### Mãos, armas e duas armas

```
Mãos seguindo os controles
 ├─ vr68  mãos seguem o controle no ritmo do óculos (não do jogo) ... ✅ ATIVO
 ├─ vr70  correção com o recentrar ................................... ✅ ATIVO
 ├─ vr82/83 mão escrevendo profundidade (laser não atravessar a arma) ❌ DESFEITO (travadas) – voltou à vr81
 ├─ vr86  BUG: "fantasma" das armas no giro → giro calculado antes das mãos ✅ ATIVO
 └─ vr86/87 nas portas/cenas (telão) as mãos ficam como o jogo põe ... ✅ ATIVO

Duas armas (cada gatilho a sua)
 ├─ (antes) cada mão só atira se o seu gatilho foi puxado nos últimos 150 ms
 ├─ vr101 log dos gatilhos: puxões perdidos e "barulho sem tiro"
 │     └─ causa: o jogo ignora o gatilho durante a animação do tiro; e atirava atrasado
 ├─ vr102 puxão guardado até a arma atirar; clarão por mão sem janela de 150 ms
 │     └─ BUG: 83 de 150 puxões perdidos (botão ficava apertado direto)
 ├─ vr103 puxão guardado vira um "toque" novo quando o jogo pode atirar
 │     └─ BUG: metade dos puxões descartados na hora (conta de tempo errada)
 └─ vr104 conta de tempo corrigida → 0 puxões perdidos ............... ✅ ATIVO (testado)
```

### Laser

```
vr77  laser vermelho fraco que some ao longo do feixe (vr_sight = 2) ✅ ATIVO
vr80  laser escondido atrás de paredes/arma (profundidade do jogo) .. ✅ ATIVO
PENDENTE: o laser ainda aparece através da arma (o jeito vr82/83 causou travadas)
```

### Recarga e munição

```
vr_manual_reload = 1
 ├─ B tira o pente; caixa no cinto (coldre); mão esquerda leva à arma ✅ ATIVO
 ├─ vr87 caixa de munição desenhada (no cinto / na mão) ............ ✅ ATIVO
 ├─ vr90 caixa: só faces viradas para o olho; abaixo do número ..... ✅ ATIVO
 ├─ vr94 BUG: faca rápida bloqueada perto do cinto sem munição → só bloqueia com munição ✅ ATIVO
 ├─ vr101 sem recarga automática, nem com a arma vazia (só clica) .. ✅ ATIVO
 ├─ vr111 BUG: Lugers do Steve (367 balas, sem munição no inventário) – B tirava o pente e não havia como pôr de volta
 │     └─ B só tira o pente se houver munição para recarregar; B de novo devolve um pente preso ✅ ATIVO
 └─ vr107 BUG: arma dourada (Lugers) com 200 de munição depois do Steve
       ├─ causa provável: as balas do pente tirado com B voltavam pelo número do
       │  espaço do inventário, sem conferir se ainda era a mesma arma/personagem
       ├─ conserto: devolve só para a mesma arma, do mesmo personagem; senão descarta ✅ ATIVO
       └─ log "vigia do inventário" (quem mudou cada item) ........... 🔍 SÓ LOG (aguardando teste)
```

### Faca

```
vr88  log do corte (qual lado do fio vai na frente) ................. 🔍 (feito; mediu 33 de 34)
vr89  só o fio corta .................................................. ✅ ATIVO
vr91  sem tempo de espera: arma de novo quando a mão desacelera ...... ✅ ATIVO
vr91  ângulo de 45° ................................................... ❌ trocado na vr93 (apertado demais)
vr93  ângulo de 60° ................................................... ✅ ATIVO
vr_slash_speed = 220 (força do golpe)
```

### Escada

```
vr91  na escada o analógico direito não gira ........................ ✅ ATIVO
vr95  na escada a visão virava de frente para a escada .............. ❌ DESFEITO na vr102 (você pediu)
```

### Vida, golpes e agarrão (vr110–vr112)

```
vr110 vibração do lado do golpe; pulso no agarrão ................... ✅ ATIVO
vr113 no agarrão a vibração segue o som da mordida (pulso só de reserva) ✅ ATIVO
vr114 só o zumbi que segura: mordida forte, agarrar/soltar médio ......... ✅ ATIVO
      └─ BUG: agarrão suposto estado 5; o log da vr111 mostrou 4 → vr112 usa 4 e 5 ✅
vr112 piscada amarela/laranja a cada golpe; roxo envenenado; vermelho+roxo ✅ ATIVO (vr_danger_fx)
vr112 som de coração com o pulso (nosso, ou heartbeat.wav na pasta) .... ✅ ATIVO (vr_heart_sound)
vr112 zumbi dentro da câmera → a câmera recua (só a câmera; mãos vão junto)
      e bordas escuras abaixo de 15 cm .............................. ⚠️ ATIVO (vr_close_push = 35)
      ├─ vr114 recuo mais suave (~0,3 s); dono subiu para 45 cm (depois voltou a 35)
      ├─ vr115 BUG: recuava para longe da cabeça do zumbi (no pescoço dela: para o lado)
      │        → recua para longe do corpo dele; empurrão por mordida opcional (vr_bite_push) ✅
      └─ log "close" mostra quanto recuou – sai quando estiver certo
```

### Painel VR e conforto (vr116)

```
vr116 página VR no menu do jogo (título e em jogo) e no F1 ............ ✅ ATIVO
      ├─ Snap turn = vr_turn · Sight = vr_sight · Resolution = vr_res
      ├─ Comfort vignette = vr_comfort (NOVA): bordas escuras só com o analógico/giro ✅ ATIVO (padrão Off)
      └─ vr118 menus de 3 escolhas: Sight Laser/Dot/Off · Comfort Off/Medium/Strong
```

### Telão (cenas, portas, menus)

```
vr87  mãos voltam quando a câmera do VR roda de novo ................ ✅ ATIVO
vr100 telão na altura dos olhos (sobe/desce só ~6°) ................. ✅ ATIVO
```

### Desempenho (travadas)

```
Travadas pequenas
 ├─ vr71–74 logs de tempo (onde o quadro gasta) .................... 🔍 SÓ LOG
 │     └─ causa (certeza): a entrega do quadro ao Virtual Desktop (xrEndFrame) trava 10–26 ms
 ├─ vr99 glFlush depois de cada olho ................................ ✅ ATIVO – NUNCA MEDIDO se ajudou
 └─ travadas com a câmera livre desenhando a sala inteira (PC_ALL_CUTS) – você pediu para não mexer
```

### Vigias (vr108) – avisos antes do bug aparecer

Arquivo `pc/glue/vr_watch.c`. Só com `vr_debug_log = 1`. Cada vigia escreve uma linha
`vr-watch: VIGIA ...` no `recvx.log` quando uma regra quebra (no máximo 12 por regra, e nas
2 primeiras vezes os últimos 15 quadros). Ao fechar o jogo e a cada 5 minutos (vr109): resumo com quantas vezes cada uma
disparou e quanto tempo os vigias gastaram (para provar que eles não causam lentidão).

```
posicao: pulo sem porta/cena ......... mais de 0,5 m num quadro (vr109: só depois de 0,5 s andando normal;
                                       2 s depois de trocar de sala; fora de escada, cena e agarrão)
direcao: giro brusco sem cena ........ mais de 90° num quadro (vr109; fora do levantar das armas, que mira sozinho)
pente pendurado ...................... pente tirado com B continua fora depois de trocar de sala/personagem
gatilho sem o dedo ................... o VR segurou "atirar" por 1 s sem nenhum gatilho puxado
mira (vr95) .......................... a vr95 desfez mais de 0,3 m do jogo num quadro (empurrão? agarrão?)
duas armas: puxão perdido ............ um puxão guardado não virou tiro em 700 ms
munição: item sem ser arma/munição ... ganhou balas (o caso das Lugers)
munição: mudou pelo jogo ............. durante o jogo, sem cena nem mensagem, e não foi um tiro
```

Custo: só comparações de números que o VR já tem, uma vez por quadro do jogo (30 por segundo),
fora do caminho do óculos; nada é escrito se nenhuma regra quebrar.

### Base do port (aikatore)

```
Base: recvx-enhanced-recomp 0.4.0
vr106 trazidos 3 consertos da 0.5.2 .................................. ✅ ATIVO
   ├─ sem travamento em 4 salas com a câmera livre
   ├─ troca de itens do baú (ItemBoxChange)
   └─ bug de um inimigo (bhEne18)
0.6.0 (17/10/2026): NÃO trazida (você decidiu deixar como está)
```

---

## 2. Lista por versão

| Versão | O que mudou | Por quê | Situação |
|---|---|---|---|
| vr64–65 | (sem registro detalhado) | – | – |
| vr66 | personagens do replay sem o deslocamento XYOFFSET | imagem desalinhada | ✅ |
| vr67 | espaço do jogador interpolado entre quadros do jogo | andar travado | ✅ |
| vr68 | mãos seguem o controle no ritmo do óculos | mãos atrasadas | ✅ |
| vr69 | (sem registro detalhado) | – | – |
| vr70 | mãos: correção com o recentrar | mãos tortas depois de recentrar | ✅ |
| vr71 | andar suavizado só no analógico; log de tempo | balanço/enjoo | ✅ / 🔍 |
| vr72 | sem suavizar quando a parede empurra; log | tremia na parede | ✅ / 🔍 |
| vr73 | posição do corpo, não da raiz; log | balanço | ✅ / 🔍 |
| vr74 | logs de quadro lento e de parada | travadas | 🔍 |
| vr75 | câmera estável | "não quero sentir giro" | 🔘 vr_cam_steady |
| vr76 | câmera estável segue só o andar; andar mirando | – | 🔘 / ✅ |
| vr77 | catraca da câmera; laser estilo RE4 | – | 🔘 / ✅ |
| vr78 | passo mirando com a parede do jogo | atravessava parede | ✅ |
| vr79 | câmera nunca à frente; andar na hora ao soltar a mira | – | 🔘 / ✅ |
| vr80 | parede do jogo para câmera e andar no quarto; laser atrás das coisas | – | 🔘 / ✅ |
| vr81 | versão base usada para desfazer a vr82–84 | – | – |
| vr82–83 | profundidade das mãos (laser) | laser atravessando a arma | ❌ travadas |
| vr84 | andar de lado (strafe) | mal-entendido | ❌ quebrou o jogo |
| vr85 | virar na hora | giro para andar para trás | 🔘 vr_move_direct |
| vr86 | giro antes das mãos; mãos no telão | fantasma das armas | ✅ |
| vr87 | mãos voltam do telão; caixa de munição | – | ✅ |
| vr88 | volta para baixo da câmera; log da faca | – | 🔘 / 🔍 |
| vr89 | faca só com o fio | – | ✅ |
| vr90 | caixa na palma, abaixo do número, faces visíveis | – | ✅ |
| vr91 | escada trava o giro; faca sem espera; câmera recomeça | – | ✅ / 🔘 |
| vr92–94 | log "andando no mesmo lugar"; faca rápida perto do cinto | bug sem parede | 🔍 / ✅ |
| vr93 | faca 60° | 45° apertado | ✅ |
| vr95 | desfaz movimentos do jogo na mira; visão na escada | jogo puxava para trás | ⚠️ / ❌ (escada) |
| vr96 | 80% só mirando | lento ao levantar arma | ✅ |
| vr97 | catraca pela direção do analógico; alcança no passo | andando no mesmo lugar | 🔘 |
| vr98 | câmera exata | – | ❌ vr99 |
| vr99 | glFlush por olho | travadas | ✅ (não medido) |
| vr100 | telão na altura dos olhos | – | ✅ |
| vr101 | sem recarga automática; log dos gatilhos | – | ✅ / ❌ log tirado vr105 |
| vr102 | puxão guardado; escada sem virar a visão | tiros perdidos; você pediu | ✅ / ✅ |
| vr103 | puxão vira toque novo | vr102 perdia puxões | ✅ |
| vr104 | conta de tempo do puxão | vr103 perdia metade | ✅ |
| vr105 | logs dos tiros removidos | já funcionava | ✅ |
| vr106 | 3 consertos da 0.5.2 | travamento em 4 salas | ✅ |
| vr107 | munição volta só para a mesma arma; vigia do inventário | Lugers com 200 balas | ✅ / 🔍 |
| vr108 | vigias (vr_watch.c): regras checadas a cada quadro, resumo ao fechar | achar bugs antes de aparecerem | 🔍 SÓ LOG |
| vr109 | vigias sem alarme falso: pulo/giro só fora de troca de sala, cena, escada, estado que não é o andar dela; giro >90° (fora do levantar as armas); puxão perdido não conta arma vazia; resumo a cada 5 min | o log da vr108 só tinha alarmes de coisas normais do jogo | 🔍 SÓ LOG |
| vr110 | vibração do lado do ataque (força dividida, frente 1 batida, trás 2, força pelo dano); pulso durante o agarrão; log do agarrão | imersão; dados para soltar-se do zumbi | ✅ / 🔍 (log sai na vr111) |
| vr111 | B só tira o pente se houver munição no inventário para aquela arma (lista do jogo); B de novo devolve um pente preso; "Exit to desktop" no menu GAMEPLAY; log da distância do inimigo aos olhos | Lugers do Steve com 367 balas presas fora (log + vigia "pente pendurado"); não havia como sair pelo menu; zumbi atravessando a câmera | ✅ / 🔍 (log "close" sai na vr112) |
| vr112 | cores da vida: amarelo/laranja piscam a cada golpe (60–119 / 30–59, limites do jogo), roxo pulsando envenenado, vermelho e roxo alternando (mais suave) envenenado+Danger; som de coração com o pulso (nosso "tutu" ou heartbeat.wav, `vr_heart_sound`); agarrão = estado 4 (e 5); câmera recua para manter o inimigo a 35 cm dos olhos (`vr_close_push`, pela parede do jogo) e bordas escurecem abaixo de 15 cm | pedidos do dono; log da vr111 (agarrão é 4; cabeça do zumbi a 3–15 cm dos olhos) | ✅ / ⚠️ recuo da câmera |
| vr113 | no agarrão, cada som que o inimigo começa (CallEnemySeMain) vira uma batida curta e forte do lado dele – a vibração acompanha o "check, check" da mordida; o pulso de 0,4 s só se nenhum som for sentido por 1,2 s; log do número de cada som no agarrão | pedido do dono depois da vr112 | ✅ / 🔍 (log sai quando a lista de sons de mordida estiver feita) |
| vr114 | no agarrão só o zumbi que segura vibra: forte na mordida (som 0), médio ao agarrar/soltar (42/34, 40/32); log dos sons tirado; recuo da câmera mais suave (~0,3 s, ida e volta); `vr_close_push` 35 → 45 no ini do dono | log da vr113: gemidos de outros zumbis vibravam; dono: "pode aumentar um pouco mais, sem enjoo" | ✅ |
| vr115 | recuo da câmera para longe do **corpo** do zumbi (a direção da vibração), não da parte mais perto; empurrão opcional a cada mordida (`vr_bite_push`, cm, padrão 0 = desligado); `vr_close_push` de volta a 35 no ini do dono | dono: "o deslocamento deve ser ao contrário de onde estou sendo atacado, como se o zumbi me empurrasse" | ✅ |
| vr116 | página **VR** no menu (OPTION → ENHANCED SETTINGS, já na tela de título; e no F1): giro (Smooth/15–90°), bordas escuras ao andar (Off/Weak/Medium/Strong, nova: só andando com o analógico ou girando), mira (Off/Dot/Laser/Laser + dot), resolução (50–200%); log "close" tirado | pedido do dono: painel de conforto antes de jogar | ✅ |
| vr117 | carregador do OpenXR (compilado por nós, `pc/CMakeLists.txt`) não desiste quando uma camada (API layer) falha ao carregar: ela é pulada e o VR abre; README: seção "game starts on the desktop" | um jogador: com o OBS aberto o jogo caía no desktop (xrCreateInstance -32: a camada do OBS não carregava e era a única) | ✅ (não testado com OBS) |
| vr118 | página VR: Sight com 3 escolhas (Laser, Dot, Off) e Comfort vignette com 3 (Off, Medium, Strong); no ini os valores antigos continuam valendo (3 aparece como Laser, 1 como Medium) | pedido do dono | ✅ |
| vr119 = **release 1.1** | `tex_hd_per_frame` (1–4, padrão 2: texturas HD convertidas por quadro; antes 4); aviso de atualização ao abrir o exe (`host/update.c`: pergunta ao GitHub o último release, janela "abrir página / continuar"; `update_check = 0` desliga; versão `RECVX_VR_VERSION` = "1.1"); README: "What's new in 1.1", "The VR page", "Update check", "HD textures and stutter" | log: entrar numa sala carregou 34 texturas HD e deu 9 quadros de 25–38 ms (4 conversões por quadro); pedido do dono: release 1.1 e aviso de update | ✅ |

---

## 3. Correções que ainda rodam (auditoria de 17/10/2026)

Conferido no código, com `vr_cam_steady = 0` e `vr_move_direct = 0`:

| Correção | Roda com as opções desligadas? | Risco |
|---|---|---|
| vr75–vr98 (toda a câmera estável) | **Não** – a função sai logo no começo | nenhum enquanto desligada |
| vr85 (virar na hora) | **Não** – só com vr_move_direct | nenhum enquanto desligada |
| vr91 recomeço da câmera depois da escada | só marca um aviso, não faz nada sem a câmera estável | nenhum |
| **vr95 desfaz os movimentos do jogo na mira** | **SIM** – roda sempre que ela mira **ou segura a faca** | ⚠️ Foi feita porque o jogo puxava ela de volta ao mirar andando. Se um dia o jogo precisar mover ela durante a mira (empurrão de inimigo, agarrão, cena), essa correção pode desfazer. **Suspeita para bugs de "ficar presa" mirando.** |
| vr80 parede do jogo no andar do quarto | sim, é do andar no quarto | baixo |
| vr79 andar na hora ao soltar a mira | sim | baixo |
| vr99 glFlush | sim | nunca medido se ajuda |
| vr92–94 detector "andando no mesmo lugar" | só log | o log da vr103 ainda mostrou um caso mirando: o jogo moveu 0,66 m, ela andou 0,20 m e "andar mirando" puxou 0,51 m de volta – **ligado à vr95, a investigar** |
| vr71–74 logs de tempo | só log | nenhum (podem ser tirados) |

---

## 4. Pendências

Mecânicas novas e ideias com tudo o que já foi decidido: ver **`VR_FUTURO.md`**.


- vr107 (munição) testada: Lugers da Claire com 0 balas, como tem que ser. Ler o resumo dos vigias da vr109.
- Investigar a vr95 (mira desfazendo os movimentos do jogo) – ver seção 3.
- Laser aparecendo através da arma.
- Textura de verdade na caixa de munição.
- Consertar vr_cam_steady e vr_move_direct antes de religar.
- Medir se o glFlush (vr99) ajudou nas travadas.
- Buracos nas salas com a câmera livre (fundo rosa para diagnóstico; neblina; sala vizinha ou relevo 3D).
- Coldres no corpo (arma na perna, faca no peito).
- Texturas ruins do pacote: "conta-gotas" (apontar o laser + clicar o analógico direito salva só aquela
  textura, com o nome do PCSX2) e a pasta `textures\_correcoes\` que vence o pacote. Adiado (17/10/2026).

## 5. Regra daqui para frente

Toda versão nova ganha uma linha na seção 2 e entra no mapa da seção 1 **antes de compilar**.
Quando algo for desfeito ou desligado, a linha original fica e ganha ❌ com o motivo.
