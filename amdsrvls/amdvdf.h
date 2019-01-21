//
// Functin definitions for VDF support
//
#include "slst.h"
int LoadVbase(char *name);
int setvalue(char *base,char *var,char *value);
int SaveVbase(char *name);
int FindVbase(PSLSTBASE vbase,char *name);
int Resolve(char *data);
void splitvar(char *full,char *base,char *var);
void findvalue(char *full,char *base,char *var);
void getvalue(PSLSTBASE base,char *var,char *value);
char *GetPathInfo(char *name);
