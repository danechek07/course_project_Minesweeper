#define _CRT_SECURE_NO_DEPRECATE
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

/* ===================================================================
   Структура поля
   =================================================================== */
   /* Field хранит размеры поля, число мин и два массива:
      - is_mine: 0 или 1 для каждой клетки (есть мина или нет)
      - count: число соседних мин (0..8) для каждой клетки
      Массивы хранятся линейно длиной rows*cols */
typedef struct {
    int rows;               // число строк
    int cols;               // число столбцов
    int mines;              // общее число мин
    unsigned char* is_mine; // массив флагов мин
    unsigned char* count;   // массив счётчиков соседних мин
} Field;

/* Преобразует координаты (r,c) в индекс линейного массива */
static inline int IDX(const Field* f, int r, int c) {
    return r * f->cols + c;
}

/* ===================================================================
   Управление памятью поля
   =================================================================== */

   /* Создаёт поле rows x cols, выделяет память и инициализирует нулями */
Field* field_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;

    Field* f = (Field*)malloc(sizeof(Field));
    if (!f) return NULL;

    f->rows = rows;
    f->cols = cols;
    f->mines = 0;

    int n = rows * cols;
    f->is_mine = (unsigned char*)calloc(n, sizeof(unsigned char));
    f->count = (unsigned char*)calloc(n, sizeof(unsigned char));

    if (!f->is_mine || !f->count) {
        free(f->is_mine);
        free(f->count);
        free(f);
        return NULL;
    }

    return f;
}

/* Освобождает память поля */
void field_free(Field* f) {
    if (!f) return;
    free(f->is_mine);
    free(f->count);
    free(f);
}

/* Очищает поле: все клетки не мины, все счётчики 0 */
void field_clear(Field* f) {
    if (!f) return;
    int n = f->rows * f->cols;
    memset(f->is_mine, 0, n);
    memset(f->count, 0, n);
    f->mines = 0;
}

/* ===================================================================
   Подсчёт соседних мин
   =================================================================== */

   /* Для каждой клетки без мины считает количество соседних мин */
void compute_counts(Field* f) {
    if (!f) return;
    int R = f->rows;
    int C = f->cols;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) { f->count[i] = 0; continue; }

            int cnt = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int rr = r + dr;
                    int cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                    }
                }
            }
            f->count[i] = (unsigned char)cnt;
        }
    }
}

/* ===================================================================
   Генерация поля по вероятности
   =================================================================== */

   /* Заполняет поле минами с вероятностью percent (0..100) */
void generate_by_probability(Field* f, int percent) {
    if (!f) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    field_clear(f);

    int R = f->rows, C = f->cols;
    int placed = 0;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if ((rand() % 100) < percent) {
                f->is_mine[i] = 1;
                ++placed;
            }
        }
    }

    f->mines = placed;
    compute_counts(f);
}

/* ===================================================================
   Вывод поля в ASCII
   =================================================================== */

   /* Печать поля:
      - show_mines = true -> показываем все мины как '*'
      - show_mines = false -> показываем '.' для 0 и цифры 1..8 */
void print_field_ascii(const Field* f, bool show_mines) {
    if (!f) return;
    int R = f->rows, C = f->cols;

    // Верхняя рамка
    printf("+");
    for (int c = 0; c < C; ++c) printf("---+");
    printf("\n");

    for (int r = 0; r < R; ++r) {
        printf("|");
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            char ch;
            if (show_mines && f->is_mine[i]) ch = '*';
            else ch = f->is_mine[i] ? '*' : (f->count[i] == 0 ? '.' : '0' + f->count[i]);
            printf(" %c |", ch);
        }
        printf("\n");

        // Разделитель строк
        printf("+");
        for (int c = 0; c < C; ++c) printf("---+");
        printf("\n");
    }
}

/* ===================================================================
   Сохранение в файл
   =================================================================== */

   /* Сохраняет поле в текстовый файл:
      Первая строка: rows cols mines
      Далее rows строк с cols символами ('M' или '0'..'8') */
bool save_field_to_file(const Field* f, const char* fname) {
    if (!f || !fname) return false;

    FILE* out = fopen(fname, "w");
    if (!out) return false;

    fprintf(out, "%d %d %d\n", f->rows, f->cols, f->mines);
    for (int r = 0; r < f->rows; ++r) {
        for (int c = 0; c < f->cols; ++c) {
            int i = IDX(f, r, c);
            fputc(f->is_mine[i] ? 'M' : '0' + f->count[i], out);
        }
        fputc('\n', out);
    }

    fclose(out);
    return true;
}

/* ===================================================================
   Валидация счётчиков
   =================================================================== */

   /* Проверяет каждый count[i] с реальным числом соседних мин */
bool validate_field(const Field* f) {
    if (!f) return false;

    int R = f->rows, C = f->cols;
    bool ok = true;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) continue;

            int cnt = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                    }
                }
            }
        }
    }

    if (ok) printf("Валидация пройдена: все счётчики корректны.\n");
    return ok;
}

/* ===================================================================
   Детерминистический солвер
   =================================================================== */

   /* Солвер. Использует внутренние inferred_mine
      и открывает безопасные клетки логически. */

bool simulate_solver_from(const Field* f, int start_r, int start_c) {
    if (!f) return false;

    int R = f->rows, C = f->cols, N = R * C;

    // state: -1 = закрыта, 0..8 = открыта
    int* state = (int*)malloc(N * sizeof(int));
    if (!state) return false;
    for (int i = 0; i < N; ++i) state[i] = -1;

    // inferred_mine[i] = логически определена мина
    unsigned char* inferred_mine = (unsigned char*)calloc(N, sizeof(unsigned char));
    if (!inferred_mine) { free(state); return false; }

    int safe_total = 0; // общее количество безопасных клеток
    for (int i = 0; i < N; ++i) if (!f->is_mine[i]) ++safe_total;

    int start_idx = IDX(f, start_r, start_c);
    if (f->is_mine[start_idx]) { free(state); free(inferred_mine); return false; }

    int* queue = (int*)malloc(N * sizeof(int));
    if (!queue) { free(state); free(inferred_mine); return false; }
    int qh = 0, qt = 0;

    state[start_idx] = f->count[start_idx];

    if (f->count[start_idx] == 0) {
        queue[qt++] = start_idx;
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int r2 = rr + dr, c2 = cc + dc;
                    if (r2 >= 0 && r2 < R && c2 >= 0 && c2 < C) {
                        int p2 = IDX(f, r2, c2);
                        if (f->is_mine[p2]) continue;
                        if (state[p2] == -1) {
                            state[p2] = f->count[p2];
                            if (f->count[p2] == 0) queue[qt++] = p2;
                        }
                    }
                }
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int p = IDX(f, r, c);
                if (state[p] < 0) continue;

                int n = state[p];
                int inferred_neighbors = 0;
                int unknown_neighbors = 0;
                int unknown_list[8], unknown_k = 0;

                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int rr = r + dr, cc = c + dc;
                        if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                            int p2 = IDX(f, rr, cc);
                            if (inferred_mine[p2]) inferred_neighbors++;
                            else if (state[p2] == -1) unknown_list[unknown_k++] = p2;
                        }
                    }
                }
                unknown_neighbors = unknown_k;

                // Правило A: все unknown = мины
                if (unknown_neighbors > 0 && n == inferred_neighbors + unknown_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (!inferred_mine[p2]) { inferred_mine[p2] = 1; changed = true; }
                    }
                }

                // Правило B: все unknown = безопасны
                if (unknown_neighbors > 0 && n == inferred_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            changed = true;
                            if (f->count[p2] == 0) queue[qt++] = p2;
                        }
                    }
                }
            }
        }
    }

    int opened = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i] && state[i] >= 0) ++opened;

    free(queue);
    free(inferred_mine);
    free(state);

    return opened == safe_total;
}

/* Проверка решаемости поля:
   - пробует все безопасные клетки как стартовые
   - если хотя бы одна стартовая клетка логически раскрывает все безопасные,
     поле считается решаемым */
bool check_solvability(const Field* f) {
    if (!f) return false;
    int R = f->rows, C = f->cols;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int idx = IDX(f, r, c);
            if (f->is_mine[idx]) continue;
            if (simulate_solver_from(f, r, c)) {
                printf("Поле решаемо детерминистически при старте из (%d, %d).\n", r, c);
                return true;
            }
        }
    }
    printf("Поле НЕ решаемо простыми локальными правилами (детерминистически).\n");
    return false;
}

/* ===================================================================
   Меню и основной цикл программы
   =================================================================== */

void print_menu() {
    printf("\n=== Меню ===\n");
    printf("1) Показать поле (с минами)\n");
    printf("2) Проверить счётчики (валидация)\n");
    printf("3) Проверка решаемости (детерминистический солвер)\n");
    printf("4) Сохранить поле в файл\n");
    printf("0) Выйти\n");
    printf("Выберите пункт: ");
}

int main(void) {
    setlocale(LC_ALL, "RUS");
    srand((unsigned)time(NULL));

    int rows = 8, cols = 8, perc = 15;

    printf("Здравствуйте! Генератор поля Сапёр (Mines generator).\n");
    printf("Введите размеры поля (строки и столбцы), например: 8 8\n");
    if (scanf("%d %d", &rows, &cols) != 2) { rows = cols = 8; int ch; while ((ch = getchar()) != '\n' && ch != EOF) {} }
    printf("Введите вероятность заполнения минами (0..100), например: 15\n");
    if (scanf("%d", &perc) != 1) { perc = 15; int ch; while ((ch = getchar()) != '\n' && ch != EOF) {} }

    Field* field = field_create(rows, cols);
    if (!field) { printf("Ошибка: память не выделена.\n"); return 1; }

    generate_by_probability(field, perc);
    printf("Поле %dx%d создано, мин: %d\n", rows, cols, field->mines);

    while (1) {
        print_menu();
        int cmd;
        if (scanf("%d", &cmd) != 1) { int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}; printf("Неверный ввод.\n"); continue; }
        switch (cmd) {
        case 1: print_field_ascii(field, true); break;
        case 2: validate_field(field); break;
        case 3: check_solvability(field); break;
        case 4: {
            char fname[260];
            printf("Введите имя файла: ");
            if (scanf("%259s", fname) == 1) {
                if (save_field_to_file(field, fname)) printf("Сохранено в %s\n", fname);
                else printf("Ошибка при сохранении.\n");
            }
            else { int ch; while ((ch = getchar()) != '\n' && ch != EOF) {} printf("Неверный ввод.\n"); }
            break;
        }
        case 0:
            field_free(field);
            printf("Вы вышли из программы.\n");
            return 0;
        default: printf("Неизвестная команда.\n"); break;
        }
    }

    field_free(field);
    return 0;
}
