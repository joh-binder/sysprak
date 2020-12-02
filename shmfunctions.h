#ifndef SYSPRAK_SHMFUNCTIONS_H
#define SYSPRAK_SHMFUNCTIONS_H

// Erzeugt ein Shared-Memory-Segment einer gegebenen Größe und gibt dessen ID zurück, oder -1 im Fehlerfall.
int shmCreate(int shmdatasize);

// Bindet das Shared-Memory-Segment einer gegebenen ID an den Adressraum an und gibt einen Pointer auf die
// Anfangsadresse zurück, oder (void *) -1 im Fehlerfall.
void *shmAttach(int shmid);

// Bindet das Shared-Memory-Segment zu einem gegebenen Pointer wieder vom Adressraum ab.
// Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück.
int shmDetach(void *shmpointer);

// Löscht das Shared-Memory-Segment zu einer gegebenen ID.
// Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück.
int shmDelete(int shmid);

#endif //SYSPRAK_SHMFUNCTIONS_H
