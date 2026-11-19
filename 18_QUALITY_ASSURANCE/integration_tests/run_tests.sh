#!/bin/sh
# Verifie que le boot reussit et que tous les modules s'initialisent.
OUT=$(./build/talco-os3) || { echo "FAIL: boot"; exit 1; }
echo "$OUT" | tail -1
echo "$OUT" | grep -q "0 en echec" || { echo "FAIL: modules en echec"; exit 1; }
echo "$OUT" | grep -q "TALCO-OS3" || { echo "FAIL: banniere"; exit 1; }
echo "PASS"
