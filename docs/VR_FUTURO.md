# Futuras mudanças do VR – mecânicas novas e ideias

Este arquivo guarda **tudo o que foi decidido para o futuro**, para não se perder quando a
conversa for limpa. Cada item diz: o que é, o que já foi decidido, o que falta descobrir,
**quais logs estão juntando informação** para ele, e a situação.

Regras que valem para todos (pedidas pelo dono do projeto):

- Nada é feito sem explicar o plano e ouvir "pode".
- Antes de mudar, descobrir a causa com dados (log, medição, cores de diagnóstico).
- **Log de informação: quando ele já tiver dado o que precisamos, ele é apagado**, para não ficar
  rodando à toa e gastando o computador.
- Mexer no jogo usando as funções do próprio jogo sempre que der (é o que menos dá bug).
- Mecânica nova = bug novo: cada uma ganha vigia (vr_watch.c) e entrada no `VR_MUDANCAS.md`.

Situação: 🟢 em andamento · 🟡 juntando dados · ⚪ ideia, não começou · ⏸ adiado pelo dono.

---

## 1. Soltar-se do zumbi (como no RE4 VR) – 🟡 juntando dados

**O que o dono quer:** quando um inimigo agarra, sacudir os controles e/ou apertar botões rápido
para se soltar. "É para ser difícil, mas possível."

**Já decidido:**
- Uma "barra de fuga" invisível: enche sacudindo os controles com força (empurrar/puxar) e
  apertando A/B rápido; esvazia devagar se parar.
- Opção `vr_escape` no `recvx.ini` para a dificuldade (0 = desligado).
- Vibração forte a cada sacudida que conta.
- Para soltar: usar **o jeito do próprio jogo de o zumbi soltar** (ex.: quando ele leva um golpe
  no meio da mordida), não teletransportar a personagem.

**O que já se sabe (código):**
- O CVX não tem "apertar para soltar": o fim do agarrão é decidido pela animação de mordida do
  inimigo (ex.: `en01.c`, tabela `en01_kamikami`); o jogo só zera o analógico enquanto ela está presa.
- Presa = `plp->mode0 == 5` (`player.c`, `bhCPM0_enedam`).

**Log juntando dados (vr110, `vr_debug_log = 1`):** `vr-diag: hold: ...` – início (qual inimigo,
de que lado), cada mudança de estado do inimigo (modos, vida, animação e quadro), fim (duração e
estado final). **Apagar esse log depois que a vr111 estiver pronta.**

**Falta:** o dono se deixar agarrar algumas vezes (zumbis, e se der outros inimigos) com a vr110;
ler o log; achar o caminho do jogo para o inimigo soltar; plano da fuga.

---

## 2. Vibração do lado do ataque – 🟢 feito na vr110 (testar)

- Força dividida entre os controles conforme o lado (só o direito num golpe totalmente à direita;
  os dois iguais de frente ou de trás).
- Ritmo: de frente uma batida forte; por trás duas batidas ("tum-tum").
- Força pelo dano.
- Pulsando do lado do inimigo durante o agarrão.
- Falta: o dono sentir e dizer se quer mais forte/fraco ou outro ritmo. Vibração grave/aguda:
  não sei se o Virtual Desktop respeita (testar se pedido).

---

## 3. Conta-gotas de texturas ruins – ⏸ adiado (17/10/2026)

**O que o dono quer:** só a textura que está ruim, uma de cada vez – não um dump de tudo.

**Já decidido:**
- Apontar o laser da mão direita na textura ruim e **clicar o analógico direito** (hoje sem uso).
- O jogo descobre a textura naquele ponto (desenhando aquele quadro mais uma vez com uma cor por
  textura e lendo o ponto – só no clique, custo zero sem clicar) e salva **só ela**: PNG com o
  nome exato do PCSX2 em `textures_dump\`, a imagem original; o log diz qual arquivo do pacote
  está no lugar; vibra para confirmar.
- O dono aumenta na IA, salva com o mesmo nome em `textures\_correcoes\` – essa pasta **sempre
  vence o pacote** (hoje, entre arquivos repetidos, vence o primeiro achado).
- Cuidados: transparência (nem toda IA mantém); aumentar por fator inteiro (2x, 4x…).

**Código útil:** `pc/src/gs/texreplace.c` (nomes PCSX2 em `names()`, `insert` = primeiro vence),
`gs_gl.c` e `textures.c` (onde a textura é decodificada; quando trocada pelo pacote, hoje não
decodifica a original – no clique teria que decodificar).

---

## 4. Buracos nas salas com a câmera livre – ⏸ para depois

**O problema:** o jogo foi feito para câmeras fixas; com a câmera livre do VR aparecem lugares sem
parede/textura. Exemplo do dono: no hall principal, subindo para o lado do cassino, olhando à
direita, a parte aberta do lado da escada tem um buraco (a escada deve ser outra sala, não carregada).

**Três tipos de buraco (descobrir qual é cada um com cores de diagnóstico):**
1. parede de um lado só (vista por trás some) → desenhar os dois lados (só visual);
2. superfície sem textura → textura nova pelo sistema do pacote;
3. nada ali (o jogo não tem a peça) → precisa de peça nova.

**Ideias, da mais simples para a mais difícil:**
- Fundo **rosa** de teste para confirmar onde não há nada desenhado.
- Fundo escuro / cor da neblina (o buraco vira escuridão).
- Neblina mais forte **só nas salas indicadas** (`vr_fog` regulável; a neblina vale para a sala
  inteira; esconde também inimigos se muito perto – medir no log a distância da neblina e do buraco).
- Imagem com relevo 3D (cor + profundidade capturadas do próprio jogo na outra sala, por botão) –
  dá profundidade para os dois olhos e para pequenos movimentos da cabeça.
- Desenhar a sala vizinha junto, só como imagem (encaixe pelas coordenadas da porta; texturas;
  custo de desenho medido no log antes). Tudo do lado do PC, fora da memória de PS2 do jogo.

**Como o dono vai ajudar:** foto do buraco; o log já diz a sala (`vr-diag: room RM_xxxx`).

---

## 6. Zumbi atravessando a câmera – ✅ feito (vr112–vr115, aprovado)

**O problema (dono):** no ataque e no agarrão o zumbi entra na câmera; dá para ver dentro do corpo
dele. A animação junta os dois corpos; no jogo original a câmera ficava longe, no VR ela está na
cabeça. A vibração está certa (segue a boca do zumbi) – o que não bate é a imagem.

**Já decidido:** **não** empurrar o zumbi ("barreirinha") – mexe na posição dele, estado do jogo
(mordida/dano/soltar dependem dos corpos colados): risco alto de bug. Mexer **só na câmera**:
1. recuar a câmera uns centímetros para longe do inimigo enquanto ele estiver colado, voltando suave;
2. se algo ainda chegar perto demais dos olhos, escurecer as bordas.

**Log juntando dados (vr111):** `vr-diag: close: ...` – a cada aproximação: o quanto o inimigo
chegou perto dos olhos (a parte do corpo mais perto), quantos quadros a menos de 60 e 30 cm, e os
estados (ataque/agarrão). **Apagar depois da vr112.**

## 7. Mods "por fora" (arquivos que o jogador coloca na pasta) – ⚪ ideia (17/10/2026)

**O que o dono quer:** coisas que se colocam por fora, sem mexer no jogo – como o `heartbeat.wav`
da vr112 (se o arquivo existir na pasta, o jogo usa ele no lugar do som nosso).

**Ideias para seguir esse modelo:** sons próprios do VR (coração, vibração com som, avisos) em
arquivos `.wav`; texturas de correção (`textures\_correcoes\`, item 3); cores e intensidade
das bordas (vida, veneno) num arquivo de configuração. Sempre: arquivo ausente = o padrão nosso;
opção 0 = desligado.

## 8. Texturas HD sem travar – ⏸ para depois (pedido em 18/10/2026)

**O problema (log da vr118):** ao entrar numa sala o jogo leu 34 texturas HD de 4 MB (um quadro de
193 ms) e converteu as sem mipmap 4 por quadro (9 quadros de 25–38 ms; o óculos quer 11 ms). Ler a
lista das 6416 texturas na abertura: 2,5 s. A vr119 baixou para 2 por quadro (`tex_hd_per_frame`).

**Guardado para depois (decisão do dono):**
- **Opção 2 – cache no disco:** guardar as texturas já convertidas (com mipmaps, no tamanho final);
  da segunda vez numa sala, carregam prontas. Alguns GB de disco.
- **Opção 3 – em segundo plano:** ler e preparar as texturas numa linha separada, fora da que desenha
  o óculos; a sala aparece com as originais e as HD entram sem travar. A melhor e a mais trabalhosa
  (pode entrar junto com a 2).

**Código:** `pc/src/gs/texreplace.c` (`trep_lookup` lê o arquivo na hora; `trep_frame_begin` converte
na fila), `gs_gl.c` (`gs_tex_to_rgba_mips`).

## 9. Aviso de atualização – ✅ feito na vr119

`pc/src/host/update.c`. **A cada release:** subir `RECVX_VR_VERSION` no código e criar a tag no
GitHub no mesmo formato (o dono usou `v1.0`; a 1.1 é `v1.1`). O jogo compara os números.

## 5. Outras pendências (do `VR_MUDANCAS.md`)

- ⚪ Laser aparecendo através da arma (o jeito vr82/83 causou travadas – precisa de outro).
- ⚪ Textura de verdade na caixa de munição.
- ⚪ Consertar `vr_cam_steady` e `vr_move_direct` antes de religar (visão virando para trás:
  `heading_update` volta a visão para a frente dela quando o controle some ou ao recentrar).
- ⚪ Medir se o glFlush (vr99) ajudou nas travadas.
- ⚪ Investigar a vr95 (mira desfaz os movimentos do jogo, roda sempre, também com a faca).
- ⚪ Coldres no corpo (arma na perna, faca no peito).
- ⚪ 0.6.0 do aikatore: **não trazer** (decisão do dono); talvez no futuro só o "carregamento
  rápido" e a "intensidade da neblina" (pequenos).
- ⚪ README em português (oferecido, não pedido ainda).

---

## Logs de informação rodando agora (apagar quando derem o que precisamos)

| Log | Para quê | Desde | Apagar quando |
|---|---|---|---|
| `vr-diag: hold:` | item 1 (soltar-se do zumbi) | vr110 | a fuga estiver pronta |
| ~~`vr-diag: close:`~~ | item 6 (zumbi na câmera) | vr111 | **tirado na vr116** |
| `vr-diag: inventory:` | bug da munição (vr107) | vr107 | – já confirmou o conserto; pode sair na próxima limpeza |
| `vr-watch: VIGIA` | avisar bugs das nossas mecânicas | vr108 | cada regra sai quando a parte dela estiver estável |
