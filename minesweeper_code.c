#define _CRT_SECURE_NO_DEPRECATE
#include <locale.h>
#include <stdio.h>
#include <stdlib.h> // работа с памятью, генерация случайных чисел, exit, malloc, free
#include <string.h> // работа со строками и памятью (memset, memcpy, strlen и т.д.)
#include <time.h>
#include <stdbool.h> // тип bool, значения true/false

/* Максимальное число попыток найти решение.
   Технически — это просто практический лимит, чтобы программа не зацикливалась
   в бесконечном цикле при неблагоприятных параметрах (например, perc=100%).
*/
#define MAX_ATTEMPTS 1000

/* ===================================================================
   Структура данных для поля "Сапёр"
   =================================================================== */

   /* Field хранит:
      - rows, cols : размеры поля
      - mines      : текущее число расставленных мин
      - is_mine    : линейный массив длины rows*cols; 1 = мина, 0 = нет
      - count      : линейный массив длины rows*cols; count[i] = число мин вокруг клетки i
   */
typedef struct {
    int rows;
    int cols;
    int mines;
    unsigned char* is_mine;
    unsigned char* count;
} Field;

/* -------------------------------------------------------------------
   Утилита: перевод (r,c) в индекс линейного массива.
   Хранение в 1D массиве упрощает выделение/освобождение памяти и
   делает код компактнее: i = r * cols + c.
   ------------------------------------------------------------------- */
static inline int IDX(const Field* f, int r, int c) {
    return r * f->cols + c;
}

/* ===================================================================
   Управление памятью поля
   =================================================================== */

   /* field_create
      - Выделяет память и инициализирует структуру поля.
      - Возвращает NULL при ошибке.
   */
Field* field_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    Field* f = (Field*)malloc(sizeof(Field));
    if (!f) return NULL;

    f->rows = rows;
    f->cols = cols;
    f->mines = 0;
    int n = rows * cols;
    f->is_mine = (unsigned char*)calloc(n, sizeof(unsigned char)); // нули
    f->count = (unsigned char*)calloc(n, sizeof(unsigned char));   // нули
    if (!f->is_mine || !f->count) { // проверка успешности выделения
        free(f->is_mine);
        free(f->count);
        free(f);
        return NULL;
    }
    return f;
}

/* field_free
   - Освобождает память структуры и её массивов.
   - Безопасно вызывать с NULL.
*/
void field_free(Field* f) {
    if (!f) return;
    free(f->is_mine);
    free(f->count);
    free(f);
}

/* field_clear
   - Очищает поле: сбрасывает флаги мин, обнуляет счётчики и счётчик мин.
   - Используется перед новой генерацией.
*/
void field_clear(Field* f) {
    if (!f) return;
    int n = f->rows * f->cols;
    memset(f->is_mine, 0, n * sizeof(unsigned char));
    memset(f->count, 0, n * sizeof(unsigned char));
    f->mines = 0;
}

/* ===================================================================
   Подсчёт счётчиков (чисел в клетках)
   =================================================================== */

   /* compute_counts
      - Для каждой клетки, если она не мина, вычисляет количество мин среди 8 соседей.
      - Результат записывается в f->count.
   */
void compute_counts(Field* f) {
    if (!f) return;
    int R = f->rows, C = f->cols;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) { f->count[i] = 0; continue; }

            int cnt = 0; // считаем соседние мины
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue; // сама клетка не считается
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C)
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                }
            f->count[i] = (unsigned char)cnt;
        }
    }
}

/* ===================================================================
   Генерация поля
   =================================================================== */

   /* generate_by_probability
      - Для каждой клетки кидает случайное число 0..99 и, если < percent,
        устанавливает там мину.
      - После расстановки мин вызывает compute_counts.
      - percent ограничен 0..100.
   */
void generate_by_probability(Field* f, int percent) {
    if (!f) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    field_clear(f);
    int R = f->rows, C = f->cols;
    int placed = 0;

    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if ((rand() % 100) < percent) { // вероятность percent%
                f->is_mine[i] = 1;
                ++placed;
            }
        }
    f->mines = placed;
    compute_counts(f);
}

/* ===================================================================
   Печать поля (ASCII)
   =================================================================== */

   /* print_field_ascii
      - Печатает поле в виде таблицы.
      - show_mines=true: мины отображаются '*'.
      - Значение 0 отображается как '.', 1..8 — цифры.
   */
void print_field_ascii(const Field* f, bool show_mines) {
    if (!f) return;

    int R = f->rows, C = f->cols;

    printf("+");
    for (int c = 0; c < C; ++c) printf("---+");
    printf("\n");

    for (int r = 0; r < R; ++r) {
        printf("|");
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            char ch;
            if (show_mines && f->is_mine[i]) ch = '*';
            else if (f->is_mine[i]) ch = '*'; // в этой программе для простоты всегда показываем мины
            else ch = (f->count[i] == 0) ? '.' : (char)('0' + f->count[i]);
            printf(" %c |", ch);
        }
        printf("\n+");
        for (int c = 0; c < C; ++c) printf("---+");
        printf("\n");
    }
}

/* ===================================================================
   Сохранение и валидация
   =================================================================== */

   /* save_field_to_file
      - Сохраняет поле в текстовом формате:
        первая строка: rows cols mines
        затем rows строк, по cols символов: 'M' или '0'..'8'
   */
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

/* validate_field
   - Для каждой неминной клетки пересчитывает число соседних мин и сравнивает
     с f->count[i]. Печатает ошибку для несоответствий и возвращает false,
     если хоть одно несоответствие найдено.
*/
bool validate_field(const Field* f) {
    if (!f) return false;
    int R = f->rows, C = f->cols;
    bool ok = true;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) continue;

            int cnt = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C)
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                }

            if (cnt != f->count[i]) {
                printf("Ошибка: клетка (%d,%d) имеет count=%d, а должно быть %d\n",
                    r, c, f->count[i], cnt);
                ok = false;
            }
        }
    }

    if (ok) printf("Валидация пройдена: все счетчики корректны.\n");
    return ok;
}

/* ===================================================================
   Детерминистический солвер (локальные правила)
   =================================================================== */

   /*
     Идея (простыми словами):
       - Мы моделируем, как играет человек, применяя только очевидные локальные правила,
         без угадывания и сложных дедукций.
       - Правила те же, что в реальной игре:
           * Если число в клетке n == (количество помеченных мин) + (количество закрытых соседних клеток),
             то все эти закрытые клетки — мины.
           * Если число в клетке n == (количество помеченных мин), то все остальные закрытые
             соседние клетки безопасны и их можно открыть.
       - Алгоритм стартует с одной стартовой клетки (как будто игрок кликнул на неё).
         Мы будем пробовать разные стартовые клетки (в check_solvability).
   */

   /* simulate_solver_from
      - Пытаемся логически раскрыть всё поле, начиная со start_r,start_c.
      - Возвращает true, если все безопасные клетки можно открыть, применяя только локальную логику.
      - Вспомогательные массивы:
          state[i]        : -1 = закрыта, >=0 = открыта и содержит число (f->count[i])
          inferred_mine[i]: 0/1 — внутреннее помечение, что клетка точно мина (не игровой флаг)
      - Использует очередь (BFS - (англ. breadth-first search, «поиск в ширину») — 
      алгоритм обхода графа в ширину.) для раскрытия нулевых областей.
   */
bool simulate_solver_from(const Field* f, int start_r, int start_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols, N = R * C;

    /* state: -1 закрыта, 0..8 открыта */
    int* state = (int*)malloc(N * sizeof(int));
    if (!state) return false;
    for (int i = 0; i < N; ++i) state[i] = -1;

    /* inferred_mine: внутренние пометки о том, что клетка наверняка мина */
    unsigned char* inferred_mine = (unsigned char*)calloc(N, sizeof(unsigned char));
    if (!inferred_mine) { free(state); return false; }

    /* сколько безопасных клеток должно быть открыто в конце */
    int safe_total = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i]) ++safe_total;

    int start_idx = IDX(f, start_r, start_c);
    if (f->is_mine[start_idx]) { free(state); free(inferred_mine); return false; }

    /* очередь для BFS раскрытия нулей */
    int* queue = (int*)malloc(N * sizeof(int));
    if (!queue) { free(state); free(inferred_mine); return false; }
    int qh = 0, qt = 0;

    /* открываем стартовую клетку */
    state[start_idx] = f->count[start_idx];
    if (f->count[start_idx] == 0) {
        /* если ноль — расширяем нулевую область */
        queue[qt++] = start_idx;
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr)
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

    /* основной итеративный цикл применения локальных правил */
    bool changed = true;
    while (changed) {
        changed = false;

        /* для каждой открытой клетки считаем соседей и пробуем применить правила */
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int p = IDX(f, r, c);
                if (state[p] < 0) continue; /* если закрыта — пропускаем */

                int n = state[p]; /* число на этой клетке */
                int inferred_neighbors = 0; /* уже помеченные внутренне как мины */
                int unknown_neighbors = 0;  /* закрытые и непомеченные соседи */
                int unknown_list[8];
                int unknown_k = 0;

                for (int dr = -1; dr <= 1; ++dr)
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

                /* Правило A: если число == known + unknown -> все unknown — мины */
                if (unknown_neighbors > 0 && n == inferred_neighbors + unknown_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (!inferred_mine[p2]) {
                            inferred_mine[p2] = 1;
                            changed = true;
                        }
                    }
                }

                /* Правило B: если число == known -> все unknown безопасны -> открываем их */
                if (unknown_neighbors > 0 && n == inferred_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            changed = true;
                            /* если новый открытый — ноль, добавляем в очередь для flood-fill */
                            if (f->count[p2] == 0) queue[qt++] = p2;
                        }
                    }
                }
            }
        }

        /* если очередь пополнилась нулями — расширяем их (BFS) */
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int r2 = rr + dr, c2 = cc + dc;
                    if (r2 >= 0 && r2 < R && c2 >= 0 && c2 < C) {
                        int p2 = IDX(f, r2, c2);
                        if (f->is_mine[p2]) continue;
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            if (f->count[p2] == 0) queue[qt++] = p2;
                            changed = true; /* открытие изменило состояние */
                        }
                    }
                }
        }

        /* после обработки очереди снова пойдёт полный проход — цикл завершится, когда
           ни одно правило не сможет ничего нового пометить/открыть */
    }

    /* Считаем открытые безопасные клетки */
    int opened = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i] && state[i] >= 0) ++opened;

    free(queue);
    free(inferred_mine);
    free(state);

    /* Возвращаем true только если открыты все безопасные клетки */
    return opened == safe_total;
}

/*
  check_solvability
  - Перебирает все неминные клетки и вызывает simulate_solver_from для каждой.
  - При первом успехе возвращает true.
  - Если ни одна стартовая клетка не дала полного решения, возвращает false..
*/
bool check_solvability(const Field* f, int* out_r, int* out_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols;
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int idx = IDX(f, r, c);
            if (f->is_mine[idx]) continue;
            if (simulate_solver_from(f, r, c)) {
                if (out_r) *out_r = r;
                if (out_c) *out_c = c;
                return true;
            }
        }
    }
    return false;
}

/* ===================================================================
   Основной цикл программы и пользовательский интерфейс
   =================================================================== */

int main(void) {
    setlocale(LC_ALL, "Rus");
    /* инициализация RNG */
    srand((unsigned)time(NULL));

    printf("Здравствуйте! Это генератор поля Сапёр (Mines generator).\n");

    for (;;) { /* внешний бесконечный цикл: после сохранения или отказа можно начать заново */
        int rows = 8, cols = 8, perc = 15;

        /* ---------- Ввод размеров поля ---------- */
        printf("\nВведите размеры поля (строки столбцы), например: 8 8\n");
        if (scanf("%d %d", &rows, &cols) != 2) {
            printf("Ввод некорректен. Установлено 8x8.\n");
            rows = 8; cols = 8;
        }
        while (getchar() != '\n'); // очистка остатка ввода

        /* ---------- Ввод вероятности мин ---------- */
        printf("Введите вероятность заполнения минами (0..100), например: 15\n");
        if (scanf("%d", &perc) != 1 || perc < 0 || perc > 100) {
            printf("Ввод некорректен. Установлено 15%%.\n");
            perc = 15;
        }
        while (getchar() != '\n');

        /* Создаём поле нужного размера */
        Field* field = field_create(rows, cols);
        if (!field) { printf("Ошибка выделения памяти.\n"); return 1; }

    generate_again:
        {
            bool solvable = false;
            int start_r = -1, start_c = -1;

            /* ---- Попытки сгенерировать решаемое поле ----
               Мы делаем до MAX_ATTEMPTS попыток. Каждая попытка:
                 - полностью пересоздаёт расположение мин,
                 - проверяет решаемость детерминистическим солвером.
               Если после MAX_ATTEMPTS ничего не найдено — считаем, что с этими
               параметрами (rows,cols,perc) найти решаемое поле быстро не получилось.
               В этом случае даём пользователю выбор: перегенерировать / ввести новые параметры / выйти.
               Примечание: MAX_ATTEMPTS — практический предел, а не теоретическая граница.
            */
            for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
                generate_by_probability(field, perc);
                if (check_solvability(field, &start_r, &start_c)) {
                    solvable = true; /* нашли подходящее поле */
                    break;
                }
            }

            /* Показываем информацию о сгенерированном поле */
            printf("\nСгенерировано поле %dx%d, вероятность %d%%, мин = %d\n",
                field->rows, field->cols, perc, field->mines);

            if (!solvable) {
                /* Если не нашли решаемое поле */
                printf("Поле НЕ решаемо детерминистическим солвером.\n");

                printf("Показать текущее поле для анализа? (Y/N): ");
                int ch = getchar();
                while (getchar() != '\n');

                if (ch == 'Y' || ch == 'y') print_field_ascii(field, true);

                printf("Поле не решаемо. Выберите: (R) сгенерировать заново, (P) ввести новые параметры, (E) выйти\n");
                int opt = getchar();
                while (getchar() != '\n');

                if (opt == 'R' || opt == 'r') {
                    /* перегенерировать с теми же параметрами */
                    field_clear(field);
                    goto generate_again;
                }
                if (opt == 'P' || opt == 'p') {
                    field_free(field);
                    continue; /* вернуться к вводу размеров и вероятности */
                }
                /* E или любое другое — выход */
                field_free(field);
                printf("Выход.\n");
                return 0;
            }
            else {
                /* Если поле оказалось решаемо — сообщаем и показываем поле. */
                printf("Поле решаемо детерминистическим солвером.\n", start_r, start_c);
                print_field_ascii(field, true);

                /* ---------------- Меню после успешной генерации ---------------- */
                printf("\n(Y) выполнить автоматическую проверку счётчиков и сохранить поле, (R) перегенерировать, (P) новые параметры, (E) выйти\n");
                int choice = getchar();
                while (getchar() != '\n');

                if (choice == 'R' || choice == 'r') {
                    field_clear(field);
                    goto generate_again;
                }
                if (choice == 'P' || choice == 'p') {
                    field_free(field);
                    continue;
                }
                if (choice == 'E' || choice == 'e') {
                    field_free(field);
                    printf("Выход.\n");
                    return 0;
                }

                if (choice == 'Y' || choice == 'y') {
                    printf("Выполняю автоматическую проверку счётчиков...\n");
                    bool ok = validate_field(field);
                    if (!ok) {
                        /* Если автоматическая проверка не пройдена — предлагаем варианты */
                        printf("Автоматическая проверка нашла несоответствия. Поле отброшено.\n");
                        printf("Выберите: (R) перегенерировать, (P) новые параметры, (E) выйти\n");
                        int o2 = getchar();
                        while (getchar() != '\n');
                        if (o2 == 'R' || o2 == 'r') { field_clear(field); goto generate_again; }
                        if (o2 == 'P' || o2 == 'p') { field_free(field); continue; }
                        field_free(field); printf("Выход.\n"); return 0;
                    }
                    else {
                        /* Сохранение поля */
                        char fname[260];
                        printf("Введите имя файла для сохранения (например field.txt): ");
                        if (scanf("%259s", fname) == 1) {
                            if (save_field_to_file(field, fname)) {
                                printf("Поле успешно сохранено в %s\n", fname);
                            }
                            else {
                                printf("Ошибка при сохранении в %s\n", fname);
                            }
                        }
                        while (getchar() != '\n'); // очистка ввода
                        print_field_ascii(field, true);

                        /* Меню после сохранения: /новое поле / выход */
                        for (;;) {
                            printf("\nМеню после сохранения:  1) Сгенерировать новое поле  2) Выйти\nВыберите: ");
                            int cmd;
                            if (scanf("%d", &cmd) != 1) {
                                int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
                                printf("Неверный ввод.\n");
                                continue;
                            }
                            if (cmd == 1) { field_free(field); break; } /* начать новый цикл */
                            else if (cmd == 2) { field_free(field); printf("Выход.\n"); return 0; }
                            else printf("Неизвестная команда.\n");
                        }
                        continue; /* вернуться к внешнему циклу, предложить новые параметры */
                    }
                }
            } /* end solvable branch */
        } /* end generate_again block */

        field_free(field);
    } /* end for(;;) */

    return 0;
}
