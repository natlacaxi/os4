# Mettre TALCO-OS3 sur GitHub

```
cd TALCO-OS3
git init -b main
git add .
git commit -m "TALCO-OS3 0.2.0"
git remote add origin https://github.com/<ton-compte>/TALCO-OS3.git
git push -u origin main
```
Puis onglet **Actions** du depot : 3 jobs se lancent (simulation + tests, noyau x86 dans QEMU, noyau ARM64 dans QEMU).
Les noyaux compiles et les captures telephone/tablette sont telechargeables dans **Artifacts**.
