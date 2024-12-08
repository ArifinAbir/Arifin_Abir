#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define MAX_WORD_LENGTH 100
#define MAX_SUGGESTIONS 5
#define Input_Id 1
#define Check_Id 2
#define Suggestion_Id 3
#define Check_Again_Id 4
#define exit_Id 5

HWND hShowSuggestion, hInputText, hWndStaic, hTitle;
HBRUSH hGreenBrush, hRedBrush, hDefaultBrush;

// Function prototypes
void AddControls(HWND);
void AddButtons(HWND);
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
int spell_check_fun(const char *word);
int suggest_words(HWND hWnd, const char *word);
int is_duplicate(char suggestions[][MAX_WORD_LENGTH], int count, const char *word);
int levenshtein_distance(const char *s1, const char *s2);
int minimum(int a, int b, int c);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW - 2);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = L"Spell_Checker";

    if (!RegisterClassW(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    CreateWindowW(L"Spell_Checker", L"Spell Checker Application",
                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  CW_USEDEFAULT, CW_USEDEFAULT, 950, 500,
                  NULL, NULL, hInstance, NULL);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        hGreenBrush = CreateSolidBrush(RGB(144, 238, 144));
        hRedBrush = CreateSolidBrush(RGB(255, 182, 193));
        hDefaultBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        AddControls(hWnd);
        AddButtons(hWnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case Check_Id:
        {
            char input[MAX_WORD_LENGTH];
            GetWindowText(hInputText, input, sizeof(input));

            // Check if input is empty
            if (strlen(input) == 0)
            {
                SetWindowText(hShowSuggestion, "Enter word to check");
                SetWindowText(hWndStaic, "");  // Clear the status box
                InvalidateRect(hInputText, NULL, TRUE);
                InvalidateRect(hWndStaic, NULL, TRUE);
                break;  // Exit early to avoid further processing
            }

            if (spell_check_fun(input) == -1)
            {
                MessageBox(hWnd, "File cannot open", "Error", MB_ICONERROR | MB_OK);
            }
            else if (spell_check_fun(input) == 1)
            {
                SetWindowText(hWndStaic, "Correct");
                SetWindowText(hShowSuggestion, "");  // Clear suggestions if word is correct
            }
            else
            {
                suggest_words(hWnd, input);
                SetWindowText(hWndStaic, "Incorrect");
            }
            InvalidateRect(hInputText, NULL, TRUE);
            InvalidateRect(hWndStaic, NULL, TRUE);
            break;
        }
        case Check_Again_Id:
        {
            SetWindowText(hInputText, "");
            SetWindowText(hShowSuggestion, "");
            SetWindowText(hWndStaic, "");
            InvalidateRect(hInputText, NULL, TRUE);
            InvalidateRect(hWndStaic, NULL, TRUE);
            break;
        }
        case exit_Id:
            PostQuitMessage(0);
            break;
        }
        break;


    case WM_CTLCOLOREDIT:
    {
        HDC hdcEdit = (HDC)wp;
        char status[10];
        GetWindowText(hWndStaic, status, sizeof(status));
        if ((HWND)lp == hInputText)
        {
            if (strcmp(status, "Correct") == 0)
            {
                SetBkColor(hdcEdit, RGB(144, 238, 144));
                return (INT_PTR)hGreenBrush;
            }
            else if (strcmp(status, "Incorrect") == 0)
            {
                SetBkColor(hdcEdit, RGB(255, 182, 193));
                return (INT_PTR)hRedBrush;
            }
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wp;
        if ((HWND)lp == hInputText || (HWND)lp == hWndStaic)
        {
            char status[10];
            GetWindowText(hWndStaic, status, sizeof(status));
            if (strcmp(status, "Correct") == 0)
            {
                SetBkColor(hdcStatic, RGB(144, 238, 144));
                return (INT_PTR)hGreenBrush;
            }
            else if (strcmp(status, "Incorrect") == 0)
            {
                SetBkColor(hdcStatic, RGB(255, 182, 193));
                return (INT_PTR)hRedBrush;
            }
        }
        break;
    }

    case WM_DESTROY:
        DeleteObject(hGreenBrush);
        DeleteObject(hRedBrush);
        DeleteObject(hDefaultBrush);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}

void AddControls(HWND hWnd)
{
    HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");

    hTitle = CreateWindowW(L"static", L"Spell Checker", WS_CHILD | WS_VISIBLE | SS_CENTER,
                           410, 20, 450, 60, hWnd, NULL, NULL, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"static", L"Enter Word:", WS_CHILD | WS_VISIBLE | SS_CENTER,
                  410, 100, 100, 40, hWnd, NULL, NULL, NULL);
    hInputText = CreateWindowW(L"edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                               510, 100, 300, 40, hWnd, (HMENU)Input_Id, NULL, NULL);

    hWndStaic = CreateWindowW(L"static", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                              600, 210, 100, 40, hWnd, NULL, NULL, NULL);

    CreateWindowW(L"static", L"Suggestions:", WS_CHILD | WS_VISIBLE | SS_CENTER,
                  410, 270, 100, 40, hWnd, NULL, NULL, NULL);
    hShowSuggestion = CreateWindowW(L"edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY | ES_MULTILINE | SS_CENTER,
                                    510, 270, 300, 100, hWnd, (HMENU)Suggestion_Id, NULL, NULL);
}

void AddButtons(HWND hWnd)
{
    CreateWindowW(L"button", L"Check Spelling", WS_CHILD | WS_VISIBLE,
                  550, 160, 200, 40, hWnd, (HMENU)Check_Id, NULL, NULL);

    CreateWindowW(L"button", L"Check Again", WS_CHILD | WS_VISIBLE,
                  550, 380, 100, 40, hWnd, (HMENU)Check_Again_Id, NULL, NULL);

    CreateWindowW(L"button", L"Exit", WS_CHILD | WS_VISIBLE,
                  670, 380, 100, 40, hWnd, (HMENU)exit_Id, NULL, NULL);
}


int spell_check_fun(const char *word)
{
    FILE *fptr = fopen("words_alpha.txt", "r");
    if (fptr == NULL)
    {
        return -1;
    }

    int found = 0;
    char dict[MAX_WORD_LENGTH];
    while (fgets(dict, sizeof(dict), fptr))
    {
        dict[strcspn(dict, "\n")] = '\0';
        if (strcasecmp(word, dict) == 0)
        {
            found = 1;
            break;
        }
    }
    fclose(fptr);

    if (found)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int suggest_words(HWND hWnd, const char *word)    // Accept hWnd as parameter
{
    FILE *fptr = fopen("words_alpha.txt", "r");
    if (fptr == NULL)
    {
        MessageBoxA(hWnd, "File cannot open", "Error", MB_ICONERROR | MB_OK);
        return -1;
    }


    char dict[MAX_WORD_LENGTH];
    int distances[MAX_SUGGESTIONS] = { MAX_WORD_LENGTH };
    char suggestions[MAX_SUGGESTIONS][MAX_WORD_LENGTH];
    int max_distance = 3;
    int suggestion_count = 0;

    while (fgets(dict, sizeof(dict), fptr))
    {
        dict[strcspn(dict, "\n")] = '\0';

        int distance = levenshtein_distance(word, dict);
        if (distance <= max_distance && !is_duplicate(suggestions, suggestion_count, dict))
        {
            if (suggestion_count < MAX_SUGGESTIONS)
            {
                strcpy(suggestions[suggestion_count], dict);
                distances[suggestion_count] = distance;
                suggestion_count++;
            }
            else
            {
                int max_idx = 0;
                for (int i = 1; i < MAX_SUGGESTIONS; i++)
                {
                    if (distances[i] > distances[max_idx]) max_idx = i;
                }
                if (distance < distances[max_idx])
                {
                    strcpy(suggestions[max_idx], dict);
                    distances[max_idx] = distance;
                }
            }
        }
    }

    if (suggestion_count > 0)
    {
        char text[500];
        strcpy(text, " ");
        for (int i = 0; i < suggestion_count; i++)
        {
            strcat(text, suggestions[i]);
            strcat(text, "    ");
        }
        SetWindowText(hShowSuggestion, text);
    }
    else
    {
        SetWindowText(hShowSuggestion, "No Suggestion Found");
    }

    fclose(fptr);
}

int is_duplicate(char suggestions[][MAX_WORD_LENGTH], int count, const char *word)
{
    for (int i = 0; i < count; i++)
    {
        if (strcasecmp(suggestions[i], word) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int levenshtein_distance(const char *s1, const char *s2)
{
    int len1 = strlen(s1), len2 = strlen(s2);
    int dp[len1 + 1][len2 + 1];

    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;

    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;
            dp[i][j] = minimum(dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost);
        }
    }
    return dp[len1][len2];
}
int minimum(int a, int b, int c)
{
    if (a < b)
    {
        return (a < c) ? a : c;
    }
    else
    {
        return (b < c) ? b : c;
    }
}
