.586
.model flat, C

.code

extern pOriginalSwap : dword
extern pOriginalPresent : dword
extern pOriginalLayer : dword
extern RenderAndLog : near
extern LogSwap : near
extern LogPresent : near
extern LogLayer : near

hkSwap proc
    pushad
    call LogSwap
    call RenderAndLog
    popad
    jmp  pOriginalSwap
hkSwap endp

hkPresent proc
    pushad
    call LogPresent
    call RenderAndLog
    popad
    jmp  pOriginalPresent
hkPresent endp

hkLayer proc
    pushad
    call LogLayer
    call RenderAndLog
    popad
    jmp  pOriginalLayer
hkLayer endp

end