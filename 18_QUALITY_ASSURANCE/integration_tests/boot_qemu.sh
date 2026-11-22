#!/bin/bash
# Demarre le noyau dans QEMU et verifie la sortie serie. Usage : boot_qemu.sh x86|aarch64
set -u
ARCH="${1:-x86}"
ELF="talco-kernel/talco-kernel-$ARCH.elf"
LOG="$(mktemp)"
[ -f "$ELF" ] || { echo "FAIL: $ELF absent (make os / make os-arm64)"; exit 1; }

if [ "$ARCH" = "x86" ]; then
  timeout 10 qemu-system-i386 -kernel "$ELF" -display none -serial "file:$LOG" -no-reboot >/dev/null 2>&1
else
  printf 'help\nls /etc\ncat /etc/release\nmkdir /x\nwrite /x/a.txt salut\ncat /x/a.txt\nuptime\n' \
    | timeout 20 qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -kernel "$ELF" >"$LOG" 2>&1
fi

tr -d '\r' < "$LOG" | sed 's/\x1b\[[0-9;]*[A-Za-z]//g' > "$LOG.clean"
echo "---- sortie serie ($ARCH) ----"; cat "$LOG.clean"; echo "------------------------------"

fail=0
check() { grep -q -- "$1" "$LOG.clean" && echo "OK   : $2" || { echo "FAIL : $2"; fail=1; }; }
check "TALCO-OS3 0.4.0" "banniere"
check "\[ok\] pret" "demarrage complet"
if [ "$ARCH" = "aarch64" ]; then
  grep -qx "salut" "$LOG.clean" && echo "OK   : ecriture/lecture de fichier" || { echo "FAIL : ecriture/lecture de fichier"; fail=1; }
  check "TALCO-OS3 0.4.0" "cat /etc/release"
fi
exit $fail
