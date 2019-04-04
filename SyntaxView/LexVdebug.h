#ifndef LEXVDEBUG_SYNTAXVIEW_H_H_ 
#define LEXVDEBUG_SYNTAXVIEW_H_H_
#include <string>
#include <list>

using namespace std;

//user param
struct VdebugRuleParam {
    int startPos;
    int endPos;
    const char *label;
    const char *content;
};

void ClearVdebugRule();
void PushVdebugRule(VdebugRuleParam *ptr);
#endif //LEXVDEBUG_SYNTAXVIEW_H_H_