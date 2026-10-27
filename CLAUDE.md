# Porte do RE Code Veronica X (VR) para o Quest 2

Esta pasta é o porte nativo para o Quest 2 (Android armeabi-v7a), no branch `android` (também no submódulo `decomp`). O que já foi feito, o que falta e como compilar estão em `PORTE_QUEST_CONTINUAR.md`. Leia antes de começar.

## Regras do dono (sempre)

- Responder em português, simples e direto.
- **Nunca afirmar sem dado.** Sem certeza, cria um log primeiro e decide depois de ler o log. Toda mudança arriscada já vai com log.
- **Pedir "pode" antes de editar ou compilar algo novo.** Descrever o que vai fazer não é aprovação. Um pedido direto do dono ("faz X") conta como "pode".
- Limitações e chutes são ditos **antes** do "pode", não depois.
- Nunca push, tag ou release no GitHub sem o dono pedir um release.
- Sem dicas de configuração (Spacewarp, Hz, resolução etc.). Desempenho se resolve no código.

## Contexto

- A versão de PC (VR no Windows) é tocada em outra conversa, na pasta `C:\Users\Neto\source\recvx-enhanced-recomp` (branch `vr-140-upstream`). Não trocar o branch de lá. O jogo de PC fica em `F:\recvx-enhanced-recomp-0.2.0-windows-x86`.
- Decisão: continuar em 32 bits até o aikatore (upstream) fazer 64 bits. 64 bits não dá mais quadros; o gargalo é o desenho (juntar desenhos, multiview, leituras da GPU).
- Todo código do Android fica atrás de `#ifdef __ANDROID__`, e a versão de PC tem que continuar compilando.
- Ferramentas: Android SDK em `C:\Users\Neto\AppData\Local\Android\Sdk` (NDK 27.1.12297006, cmake 3.22.1, build-tools 36) e o Quest pelo adb no Wi-Fi `192.168.137.35:5555`. Para o PC: MSYS2 em `C:\msys64` e `C:\Users\Neto\source\tools`.
