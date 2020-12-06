#ifndef SYSPRAK_CONFIG_H
#define SYSPRAK_CONFIG_H

struct cnfgInfo {
    char gameKindName[20];
    unsigned int portNumber;
    char hostName[20];
};

// Hilfsnachricht zu den geforderten Kommandozeilenparametern
void printHelp(void);

// erzeugt ein neues struct cnfgInfos und initialisiert es mit Standardwerten
struct cnfgInfo createConfigStruct(void);

/* nimmt als Parameter ein Pointer auf struct cnfgInfo und einen Dateipfad entgegen;
liest die Datei und schreibt die passenden Informationen in das Struct;
gibt im Fehlerfall -1 zurück, sonst 0 */
int readFromConfFile(struct cnfgInfo *configInfo, char *path);

#endif //SYSPRAK_CONFIG_H