# Porte para o Quest 2 – como continuar numa conversa nova

Cole o texto abaixo na conversa nova.

---

Continue o porte nativo do RE Code Veronica X (VR) para o Quest 2. Leia primeiro o `CLAUDE.md` desta pasta (as regras do dono). Trabalhe SOMENTE nesta pasta, `F:\Projetos cloude\Recvx-vr` (branch `android` no repositório e no submódulo `decomp`). Não troque o branch de `C:\Users\Neto\source\recvx-enhanced-recomp`, que é o do PC (outra conversa).

**Estado:** o APK abre no Quest em VR imersivo e entra no 3D em primeira pessoa (cela da abertura, ~30 fps com `scale = 1`). Os controles Touch funcionam.

**Próximos problemas, em ordem:**
1. A tela de item/inventário (`inventory shows item`) cai para 6-9 fps e parece travada. A causa provável é uma leitura da GPU (`glReadPixels`/StoreImage), que no Adreno força sincronização: MEDIR antes de mudar. Nessa lentidão o log mostra o A "preso", porque a entrada só é lida em `read_actions`, no quadro do OpenXR (`pc/src/host/xr.c`).
2. Os personagens aparecem sem braços nas cenas. Acontece só no Quest; no PC os braços aparecem. Investigar: VU1/native, os shaders GLSL ES convertidos em `compile()` de `pc/src/gs/gs_gl.c` e as clip distances (EXT_clip_cull_distance).
3. A imagem 3D leva 9 ms por olho em média e 26 ms no pior caso (o limite é 13,9 ms a 72 Hz). O óculos perde quadros.
4. Depois: texturas HD (o Quest não lê DXT/BGRA), o abort ao fechar o app e o ícone.

**O que foi feito (commits no branch `android`):**
- `PS2_WINDOW` vale 0x30000000 no PC e 0x40000000 no Android, porque num app Android o 0x30000000 é ocupado pelo heap do Java.
- `pc/android/loader.c` reserva o endereço 0x40000000 e carrega `librecvx.so` com `android_dlopen_ext`.
- `RecvxActivity.java` é uma SDLActivity que passa `--save-dir`, `--log=`, `--no-launcher` e `--vr`, e guarda permissão persistente para a ISO em `content://`.
- `AndroidManifest.xml`: app de VR, com OpenXR.
- `build_apk.py` monta o APK com javac, d8, aapt2, zipalign e apksigner.
- GLES 3.2: os shaders são convertidos para `320 es` e foi feito o ImGui ES3.
- Os literais de texto ficam graváveis graças a `--rosegment` e ao `memory.c`.
- `xr.c` usa EGL e o loader do OpenXR com a VM e a activity.
- A resolução fica limitada a no máximo 2x, ou ao `scale` do ini.
- `log.c`: em caso de crash grava registradores e endereços da pilha. O offset na .so é o endereço − 0x40000000; use `llvm-addr2line` na versão sem strip.
- Logs `android: last second ...` (uma vez por segundo) e `android: buttons ...` (a cada troca de botão).

**Build:**
1. Nesta pasta, configure `build-android` com o cmake do SDK (`Android\Sdk\cmake\3.22.1`): `-G Ninja`, toolchain do NDK 27 (`build/cmake/android.toolchain.cmake`), `-DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-29 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRECVX_VR=ON`.
2. Rode `python pc/android/build_apk.py`.
3. Instale com `adb -s 192.168.137.35:5555 install -r --abi armeabi-v7a build-android/recvx-quest.apk`. No Git Bash, use `MSYS_NO_PATHCONV=1`.

O jogo só roda com o óculos no rosto. O log fica no Quest em `/sdcard/Android/data/org.recvx.vr/files/recvx.log`. A versão de PC tem que continuar compilando.

**Pronto quando:** dá para jogar a primeira sala e abrir o inventário em velocidade normal, sem braços faltando. Cada mudança vai com log, e a causa medida aparece antes da mudança.
