#ifndef SYSPRAK_CONFIG_H
#define SYSPRAK_CONFIG_H

#define MAX_LENGTH_GAMEKINDNAME 20
#define MAX_LENGTH_HOSTNAME 50

struct cnfgInfo {
    char gameKindName[MAX_LENGTH_GAMEKINDNAME];
    unsigned int portNumber;
    char hostName[MAX_LENGTH_HOSTNAME];
};

// Hilfsnachricht zu den geforderten Kommandozeilenparametern
void printHelp(void);

/* nimmt als Parameter ein Pointer auf struct cnfgInfo und einen Dateipfad entgegen;
liest die Datei und schreibt die passenden Informationen in das Struct;
gibt im Fehlerfall -1 zurück, sonst 0 */
int readFromConfFile(struct cnfgInfo *configInfo, char *path);

#endif //SYSPRAK_CONFIG_H