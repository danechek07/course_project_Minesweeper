/*
  mines_simple.c
  Генератор поля "Сапёр" (Minesweeper) с ASCII-отрисовкой,
  валидацией счётчиков и детерминистическим солвером.

  Что делает при старте:
   - Просит размеры поля (строки и столбцы) и вероятность мин (0..100).
   - Генерирует поле по вероятности.
   - Сообщает, что поле сгенерировано.
   - Затем показывает меню (switch-case) с опциями:
       1) Показать поле (с минами)
       2) Проверить счётчики (валидация)
       3) Проверка решаемости (детерминистический солвер, без игровых флагов)
       4) Сохранить поле в файл
       0) Выйти (печатает "Вы вышли из программы")
*/
#define _CRT_SECURE_NO_DEPRECATE
#include <locale.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

/* -------------------- Структура поля -------------------- */
/* Field хранит размеры поля, число мин и два массива:
   is_mine: 0 или 1 для каждой клетки (есть мина или нет)
   count: число соседних мин (0..8) для каждой клетки
   Массивы хранятся длиной rows*cols */
typedef struct {
    int rows;
    int cols;
    int mines;
    unsigned char* is_mine;
    unsigned char* count;
} Field;

/* Функция: преобразует (r,c) в индекс массива */
static inline int IDX(const Field* f, int r, int c) {
    return r * f->cols + c;
}
//f -> cols - f указатель на структуру Field (Экивавалетно (*f).cols)
/* -------------------- Управление памятью -------------------- */

/* Создаёт поле rows x cols, выделяет массивы и инициализирует нулями */
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

/* Делаем поле пустым: все клетки не мины и счётчики 0 */
void field_clear(Field* f) {
    if (!f) return;
    int n = f->rows * f->cols;
    memset(f->is_mine, 0, n * sizeof(unsigned char));
    memset(f->count, 0, n * sizeof(unsigned char));
    f->mines = 0;
}

/* -------------------- Подсчёт счётчиков -------------------- */

/* Для каждой клетки, не являющейся миной, считаем количество мин в 8 соседях */
void compute_counts(Field* f) {
    if (!f) return;
    int R = f->rows, C = f->cols;
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) {
                f->count[i] = 0; // для мины можно хранить 0
                continue;
            }
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

/* -------------------- Генерация поля -------------------- */

/* Заполняет поле минами по вероятности percent (0..100) */
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
            int rnd = rand() % 100; // 0..99
            if (rnd < percent) {
                f->is_mine[i] = 1;
                ++placed;
            }
        }
    }
    f->mines = placed;
    compute_counts(f);
}

/* -------------------- Вывод в ASCII -------------------- */

/* Печать поля: если show_mines == true, показываем '*' на минах,
   иначе показываем '.' для 0 и цифры для 1..8 */
void print_field_ascii(const Field* f, bool show_mines) {
    if (!f) return;
    int R = f->rows, C = f->cols;
    // верхняя рамка
    printf("+");
    for (int c = 0; c < C; ++c) printf("---+");
    printf("\n");
    for (int r = 0; r < R; ++r) {
        printf("|");
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            char ch;
            if (show_mines && f->is_mine[i]) {
                ch = '*';
            }
            else {
                if (f->is_mine[i]) {
                    ch = '*';
                }
                else {
                    unsigned char v = f->count[i];
                    ch = (v == 0) ? '.' : '0' + v;
                }
            }
            printf(" %c |", ch);
        }
        printf("\n");
        // разделитель строк
        printf("+");
        for (int c = 0; c < C; ++c) printf("---+");
        printf("\n");
    }
}

/* -------------------- Сохранение в файл -------------------- */

/* Сохранение в удобном текстовом формате:
   первая строка: rows cols mines
   затем rows строк с cols символами: 'M' или '0'..'8' */
bool save_field_to_file(const Field* f, const char* fname) {
    if (!f || !fname) return false;
    FILE* out = fopen(fname, "w");
    if (!out) return false;
    fprintf(out, "%d %d %d\n", f->rows, f->cols, f->mines);
    for (int r = 0; r < f->rows; ++r) {
        for (int c = 0; c < f->cols; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) fputc('M', out);
            else fputc('0' + f->count[i], out);
        }
        fputc('\n', out);
    }
    fclose(out);
    return true;
}

/* -------------------- Валидация счётчиков -------------------- */

/* Сравниваем каждый count[i] с реальным количеством соседних мин */
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
                    int rr = r + dr;
                    int cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                    }
                }
            }
            if (cnt != f->count[i]) {
                printf("Ошибка: клетка (%d,%d) имеет count=%d, а должно быть %d\n",
                    r, c, f->count[i], cnt);
                ok = false;
            }
        }
    }
    if (ok) printf("Валидация пройдена: все счётчики корректны.\n");
    return ok;
}

/* -------------------- Детерминистический солвер (без "флагов" в интерфейсе) -------------------- */
/*
  Важно: в ТЗ флаги не упоминаются, поэтому солвер здесь НЕ использует игровые флаги,
  но он всё равно должен *нутренно* запоминать вывод про то, какие клетки точно мины,
  чтобы не открывать их по ошибке. Это не "игровые флаги", это внутренний вывод логики.

  Алгоритм:
    - Состояние state: -1 = закрыта, >=0 = открыта (значение равно count).
    - inferred_mine[i] = true означает: по логике мы однозначно вывели, что эта клетка - мина.
      Это внутреннее знание (не показывается игроку).
    - Начинаем с открытия одной стартовой клетки (мы будем пробовать несколько стартов,
      но в основной функции проверяем все безопасные клетки как стартовые).
    - Если открытая клетка имеет count==0 — делаем обычный flood-fill и открываем соседей.
    - Потом итеративно применяем простые правила:
        A) если число в открытой клетке n == inferred_mine_neighbors + unknown_neighbors,
           то все unknown_neighbors — мины (ставим inferred_mine = true).
        B) если число n == inferred_mine_neighbors, то все unknown_neighbors безопасны -> открываем их.
    - Повторяем до сходимости.
    - В конце проверяем: открыты ли все безопасные клетки? Если да — успех.
*/

bool simulate_solver_from(const Field* f, int start_r, int start_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols, N = R * C;
    // state: -1 closed, 0..8 opened
    int* state = (int*)malloc(N * sizeof(int));
    if (!state) return false;
    for (int i = 0; i < N; ++i) state[i] = -1;

    // inferred_mine: внутренне выведенные мины (true = точно мина)
    unsigned char* inferred_mine = (unsigned char*)calloc(N, sizeof(unsigned char));
    if (!inferred_mine) { free(state); return false; }

    // сколько безопасных клеток (не мины) должно быть открыто
    int safe_total = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i]) ++safe_total;

    int start_idx = IDX(f, start_r, start_c);
    if (f->is_mine[start_idx]) { // старт — мина => неудача (игрок бы взорвался)
        free(state); free(inferred_mine); return false;
    }

    // Вспомогательная очередь для flood-fill (простая реализация на массиве)
    int* queue = (int*)malloc(N * sizeof(int));
    if (!queue) { free(state); free(inferred_mine); return false; }
    int qh = 0, qt = 0;

    // Открываем стартовую клетку
    state[start_idx] = f->count[start_idx];
    if (f->count[start_idx] == 0) {
        // добавляем в очередь для BFS раскрытия нулей
        qh = 0; qt = 0; queue[qt++] = start_idx;
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int r2 = rr + dr, c2 = cc + dc;
                    if (r2 >= 0 && r2 < R && c2 >= 0 && c2 < C) {
                        int p2 = IDX(f, r2, c2);
                        if (f->is_mine[p2]) continue;        // мины не открываем
                        if (state[p2] == -1) {              // если ещё закрыта
                            state[p2] = f->count[p2];      // открываем
                            if (f->count[p2] == 0) queue[qt++] = p2; // если 0 — добавляем для расширения
                        }
                    }
                }
            }
        }
    }

    // Основной итеративный цикл применения правил
    bool changed = true;
    while (changed) {
        changed = false;
        // Проходим все клетки. На каждой открытой применяем правила.
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int p = IDX(f, r, c);
                if (state[p] < 0) continue; // не открыта — пропуск
                int n = state[p]; // число в клетке
                int inferred_neighbors = 0; // сколько соседей мы уже пометили как inferred mine
                int unknown_neighbors = 0;  // сколько соседей закрыто и не помечено как inferred mine
                int unknown_list[8];
                int unknown_k = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int rr = r + dr, cc = c + dc;
                        if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                            int p2 = IDX(f, rr, cc);
                            if (inferred_mine[p2]) inferred_neighbors++;
                            else if (state[p2] == -1) {
                                unknown_list[unknown_k++] = p2;
                                unknown_neighbors++;
                            }
                        }
                    }
                }

                // Правило A: если n == inferred_neighbors + unknown_neighbors,
                // то все unknown_neighbors — мины (внутренне помечаем inferred_mine)
                if (unknown_neighbors > 0 && n == inferred_neighbors + unknown_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (!inferred_mine[p2]) {
                            inferred_mine[p2] = 1;
                            changed = true;
                        }
                    }
                }

                // Правило B: если n == inferred_neighbors, то все unknown_neighbors безопасны -> открываем их
                if (unknown_neighbors > 0 && n == inferred_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            // открываем p2
                            state[p2] = f->count[p2];
                            changed = true;
                            // если открытая клетка ноль — делаем flood-fill от неё
                            if (f->count[p2] == 0) {
                                // простой BFS раскрытия нулей
                                qh = 0; qt = 0;
                                queue[qt++] = p2;
                                while (qh < qt) {
                                    int cur = queue[qh++];
                                    int rr = cur / C, cc = cur % C;
                                    for (int dr2 = -1; dr2 <= 1; ++dr2) {
                                        for (int dc2 = -1; dc2 <= 1; ++dc2) {
                                            if (dr2 == 0 && dc2 == 0) continue;
                                            int r3 = rr + dr2, c3 = cc + dc2;
                                            if (r3 >= 0 && r3 < R && c3 >= 0 && c3 < C) {
                                                int p3 = IDX(f, r3, c3);
                                                if (f->is_mine[p3]) continue;
                                                if (state[p3] == -1 && !inferred_mine[p3]) {
                                                    state[p3] = f->count[p3];
                                                    if (f->count[p3] == 0) queue[qt++] = p3;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } // end for c
        } // end for r
    } // end while(changed)

    // Считаем, сколько безопасных клеток открыто
    int opened = 0;
    for (int i = 0; i < N; ++i) {
        if (!f->is_mine[i] && state[i] >= 0) ++opened;
    }

    free(queue);
    free(inferred_mine);
    free(state);

    // Если открыты все безопасные клетки — считаем, что поле решаемо детерминистически
    return opened == safe_total;
}

/* Проверка решаемости: пробуем стартовать с каждой безопасной клетки.
   Если хотя бы с одной стартовой клетки логика открывает все безопасные клетки —
   поле считаем решаемым. */
bool check_solvability(const Field* f) {
    if (!f) return false;
    int R = f->rows, C = f->cols;
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int idx = IDX(f, r, c);
            if (f->is_mine[idx]) continue; // нельзя стартовать с мины
            if (simulate_solver_from(f, r, c)) {
                printf("Поле решаемо детерминистически при старте из (%d, %d).\n", r, c);
                return true;
            }
        }
    }
    printf("Поле НЕ решаемо простыми локальными правилами (детерминистически).\n");
    return false;
}

/* -------------------- Меню и main -------------------- */

/* Печать меню (после того, как поле создано) */
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
    //SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL , "RUS");
    srand((unsigned)time(NULL)); // инициализация генератора случайных чисел

    // 1) Начальный диалог: спрашиваем размеры и вероятность
    int rows = 8, cols = 8, perc = 15;
    printf("Здравствуйте! Это генератор поля Сапёр (Mines generator).\n");
    printf("Введите размеры поля (строки и столбцы), например: 8 8\n");
    if (scanf("%d %d", &rows, &cols) != 2) {
        printf("Ввод некорректен. Используются размеры по умолчанию 8x8.\n");
        rows = 8; cols = 8;
        // очищаем ввод если было лишнее
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
    if (rows <= 0) rows = 8;
    if (cols <= 0) cols = 8;
    printf("Введите вероятность заполнения минами (0..100), например: 15\n");
    if (scanf("%d", &perc) != 1) {
        printf("Ввод некорректен. Используется 15%%.\n");
        perc = 15;
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
    if (perc < 0) perc = 0;
    if (perc > 100) perc = 100;

    // 2) Создаём поле и генерируем
    Field* field = field_create(rows, cols);
    if (!field) {
        printf("Ошибка: не удалось выделить память для поля.\n");
        return 1;
    }
    generate_by_probability(field, perc);
    printf("Поле %dx%d с вероятностью %d%% заполнения минами создано. Мин: %d\n",
        field->rows, field->cols, perc, field->mines);

    // 3) Теперь показываем компактное меню в цикле (switch-case)
    while (1) {
        print_menu();
        int cmd;
        if (scanf("%d", &cmd) != 1) {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
            printf("Неверный ввод. Попробуйте снова.\n");
            continue;
        }
        switch (cmd) {
        case 1:
            print_field_ascii(field, true);
            break;
        case 2:
            validate_field(field);
            break;
        case 3:
            check_solvability(field);
            break;
        case 4: {
            char fname[260];
            printf("Введите имя файла для сохранения (например field.txt): ");
            if (scanf("%259s", fname) == 1) {
                if (save_field_to_file(field, fname)) printf("Сохранено в %s\n", fname);
                else printf("Ошибка при сохранении в %s\n", fname);
            }
            else {
                int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
                printf("Неверный ввод имени файла.\n");
            }
            break;
        }
        case 0:
            field_free(field);
            printf("Вы вышли из программы.\n");
            return 0;
        default:
            printf("Неизвестная команда. Попробуйте снова.\n");
            break;
        } // end switch
    } // end while

    field_free(field);
    return 0;
}
