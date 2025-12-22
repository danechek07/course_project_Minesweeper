#define _CRT_SECURE_NO_DEPRECATE
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

/* ===================================================================
   Структура данных для поля "Сапёр"
   =================================================================== */

   /* Структура Field хранит всю информацию о поле:
      - rows, cols — размеры поля (строки и столбцы)
      - mines — количество мин на поле
      - is_mine — массив (размер rows*cols), где 1 = мина, 0 = нет
      - count — массив (размер rows*cols), хранит количество мин вокруг каждой клетки
   */
typedef struct {
    int rows;
    int cols;
    int mines;
    unsigned char* is_mine;
    unsigned char* count;
} Field;

/* Макрос для преобразования координат (r,c) в индекс массива:
   используется для удобного доступа к линейному массиву вместо двумерного
*/
static inline int IDX(const Field* f, int r, int c) {
    return r * f->cols + c;
}

/* ===================================================================
   Работа с памятью поля
   =================================================================== */

   /* Создание нового поля с выделением памяти для мин и счётчиков */
Field* field_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    Field* f = (Field*)malloc(sizeof(Field));
    if (!f) return NULL;

    f->rows = rows;
    f->cols = cols;
    f->mines = 0;
    int n = rows * cols;
    f->is_mine = (unsigned char*)calloc(n, sizeof(unsigned char)); // 0 = нет мины
    f->count = (unsigned char*)calloc(n, sizeof(unsigned char));   // счетчик мин вокруг
    if (!f->is_mine || !f->count) { // проверка успешности выделения памяти
        free(f->is_mine);
        free(f->count);
        free(f);
        return NULL;
    }
    return f;
}

/* Освобождение памяти поля */
void field_free(Field* f) {
    if (!f) return;
    free(f->is_mine);
    free(f->count);
    free(f);
}

/* Очистка поля: удаление мин и сброс всех счетчиков */
void field_clear(Field* f) {
    if (!f) return;
    int n = f->rows * f->cols;
    memset(f->is_mine, 0, n * sizeof(unsigned char));
    memset(f->count, 0, n * sizeof(unsigned char));
    f->mines = 0;
}

/* ===================================================================
   Подсчет количества мин вокруг каждой клетки
   =================================================================== */

   /* Для каждой клетки, если это не мина, считаем количество мин вокруг.
      Используем вложенные циклы dr и dc для обхода соседних клеток.
   */
void compute_counts(Field* f) {
    if (!f) return;
    int R = f->rows, C = f->cols;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) { f->count[i] = 0; continue; }

            int cnt = 0; // счетчик соседних мин
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue; // пропускаем саму клетку
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C)
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                }
            f->count[i] = (unsigned char)cnt;
        }
    }
}

/* ===================================================================
   Генерация случайного поля по вероятности
   =================================================================== */

   /* Генерация мин с вероятностью percent% для каждой клетки */
void generate_by_probability(Field* f, int percent) {
    if (!f) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    field_clear(f); // очищаем поле перед генерацией
    int R = f->rows, C = f->cols;
    int placed = 0; // количество расставленных мин

    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if ((rand() % 100) < percent) { // сравниваем с вероятностью
                f->is_mine[i] = 1;
                ++placed;
            }
        }
    f->mines = placed;
    compute_counts(f); // пересчитываем счетчики после расстановки мин
}

/* ===================================================================
   Визуализация поля
   =================================================================== */

   /* Выводим поле в ASCII, если show_mines = true, то мины отображаются */
void print_field_ascii(const Field* f, bool show_mines) {
    if (!f) return;

    int R = f->rows, C = f->cols;

    /* Верхняя рамка */
    printf("+");
    for (int c = 0; c < C; ++c) printf("---+");
    printf("\n");

    /* Каждая строка поля */
    for (int r = 0; r < R; ++r) {
        printf("|");
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            char ch;
            if (show_mines && f->is_mine[i]) ch = '*';
            else if (f->is_mine[i]) ch = '*';
            else ch = (f->count[i] == 0) ? '.' : (char)('0' + f->count[i]);
            printf(" %c |", ch);
        }
        printf("\n+");
        for (int c = 0; c < C; ++c) printf("---+");
        printf("\n");
    }
}

/* ===================================================================
   Сохранение и проверка поля
   =================================================================== */

   /* Сохраняем поле в текстовый файл */
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

/* Проверка корректности счетчиков поля */
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
   Детерминистический солвер
   =================================================================== */

   /*
   Алгоритм солвера (детерминистический):
   1. Выбираем стартовую клетку (которая точно не мина).
   2. Открываем стартовую клетку.
      Если её счетчик равен 0 — добавляем в очередь для BFS обхода пустых клеток.
   3. Для каждой открытой клетки:
      - Считаем количество соседних мин, которые уже определены
      - Считаем количество соседних неизвестных клеток
   4. Применяем простые правила:
      a) Если число соседних мин + число неизвестных клеток = счетчик клетки, то все неизвестные — мины
      b) Если число соседних мин = счетчик клетки, то все неизвестные клетки безопасны
   5. Если нашли новые безопасные клетки — добавляем в очередь для раскрытия смежных пустых клеток (BFS)
   6. Повторяем, пока новые клетки можно открывать
   7. После завершения проверяем, открыты ли все безопасные клетки.
      - Если да — поле решаемо детерминистически
      - Если нет — поле требует угадывания
   */

bool simulate_solver_from(const Field* f, int start_r, int start_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols, N = R * C;

    int* state = (int*)malloc(N * sizeof(int)); // состояние клеток: -1=не открыта, >=0 = открыта
    if (!state) return false;
    for (int i = 0; i < N; ++i) state[i] = -1;

    unsigned char* inferred_mine = (unsigned char*)calloc(N, sizeof(unsigned char)); // 1=мину определили
    if (!inferred_mine) { free(state); return false; }

    int safe_total = 0; // всего безопасных клеток
    for (int i = 0; i < N; ++i) if (!f->is_mine[i]) ++safe_total;

    int start_idx = IDX(f, start_r, start_c);
    if (f->is_mine[start_idx]) { free(state); free(inferred_mine); return false; }

    int* queue = (int*)malloc(N * sizeof(int)); // очередь BFS для открытия клеток
    if (!queue) { free(state); free(inferred_mine); return false; }
    int qh = 0, qt = 0;

    state[start_idx] = f->count[start_idx];
    if (f->count[start_idx] == 0) {
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

    bool changed = true;
    while (changed) { // итеративное применение правил солвера
        changed = false;
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int p = IDX(f, r, c);
                if (state[p] < 0) continue;

                int n = state[p];
                int inferred_neighbors = 0; // количество соседей, уже помеченных как мины
                int unknown_neighbors = 0;  // количество соседей, еще не известных
                int unknown_list[8];
                int unknown_k = 0;

                for (int dr = -1; dr <= 1; ++dr)
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int rr = r + dr, cc = c + dc;
                        if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                            int p2 = IDX(f, rr, cc);
                            if (inferred_mine[p2]) inferred_neighbors++;
                            else if (state[p2] == -1) { unknown_list[unknown_k++] = p2; unknown_neighbors++; }
                        }
                    }

                // Правило 1: если счетчик равен сумме мин и неизвестных, все неизвестные — мины
                if (unknown_neighbors > 0 && n == inferred_neighbors + unknown_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (!inferred_mine[p2]) {
                            inferred_mine[p2] = 1;
                            changed = true;
                        }
                    }
                }

                // Правило 2: если счетчик равен числу известных мин, все неизвестные безопасны
                if (unknown_neighbors > 0 && n == inferred_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            changed = true;
                            if (f->count[p2] == 0) queue[qt++] = p2; // добавляем пустые клетки в очередь
                        }
                    }
                }
            }
        }
    }

    int opened = 0; // считаем открытые безопасные клетки
    for (int i = 0; i < N; ++i) if (!f->is_mine[i] && state[i] >= 0) ++opened;

    free(queue);
    free(inferred_mine);
    free(state);

    return opened == safe_total; // true если все безопасные клетки открыты
}

/* Проверка, решаемо ли поле детерминистически */
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
   Основной цикл программы и пользовательский интерфейс
   =================================================================== */

int main(void) {
    setlocale(LC_ALL, "Rus");
    srand((unsigned)time(NULL));

    printf("Здравствуйте! Это генератор поля Сапёр (Mines generator).\n");

    for (;;) {
        int rows = 8, cols = 8, perc = 15;

        // Ввод размеров поля
        printf("\nВведите размеры поля (строки столбцы), например: 8 8\n");
        if (scanf("%d %d", &rows, &cols) != 2) {
            printf("Ввод некорректен. Установлено 8x8.\n");
            rows = 8; cols = 8;
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        }
        if (rows <= 0) rows = 8;
        if (cols <= 0) cols = 8;

        // Ввод вероятности мин
        printf("Введите вероятность заполнения минами (0..100), например: 15\n");
        if (scanf("%d", &perc) != 1) {
            printf("Ввод некорректен. Установлено 15%%.\n");
            perc = 15;
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        }
        if (perc < 0) perc = 0;
        if (perc > 100) perc = 100;

        Field* field = field_create(rows, cols);
        if (!field) { printf("Ошибка выделения памяти.\n"); return 1; }

    generate_again:
        generate_by_probability(field, perc);
        printf("\nСгенерировано поле %dx%d, вероятность %d%%, мин = %d\n",
            field->rows, field->cols, perc, field->mines);

        // Проверка решаемости поля
        bool solvable = check_solvability(field);

        if (!solvable) {
            // Если поле не решаемо
            printf("Показать поле для анализа? (Y/N): ");
            char ch;
            do { ch = getchar(); } while (ch == '\n' || ch == ' ' || ch == '\t');
            int junk; while ((junk = getchar()) != '\n' && junk != EOF) {}
            if (ch == 'Y' || ch == 'y') print_field_ascii(field, true);

            printf("Поле не решаемо. Выберите: (R) сгенерировать заново, (P) ввести новые параметры, (E) выйти\n");
            char opt;
            do { opt = getchar(); } while (opt == '\n' || opt == ' ' || opt == '\t');
            int junk2; while ((junk2 = getchar()) != '\n' && junk2 != EOF) {}
            if (opt == 'R' || opt == 'r') { goto generate_again; }
            if (opt == 'P' || opt == 'p') { field_free(field); continue; }
            if (opt == 'E' || opt == 'e') { field_free(field); printf("Выход.\n"); return 0; }
            field_free(field); return 0;
        }

        // Если поле решаемо
        printf("\nПоле решаемо детерминистическим солвером.\n");
        print_field_ascii(field, true);

        // Меню для проверки счетчиков и сохранения
        printf("\nПосле визуального осмотра нажмите: (Y) выполнить автоматическую проверку счётчиков и сохранить поле, (R) перегенерировать, (P) новые параметры, (E) выйти\n");
        char choice;
        do { choice = getchar(); } while (choice == '\n' || choice == ' ' || choice == '\t');
        int junk3; while ((junk3 = getchar()) != '\n' && junk3 != EOF) {}
        if (choice == 'R' || choice == 'r') { field_clear(field); goto generate_again; }
        if (choice == 'P' || choice == 'p') { field_free(field); continue; }
        if (choice == 'E' || choice == 'e') { field_free(field); printf("Выход.\n"); return 0; }

        if (choice == 'Y' || choice == 'y') {
            printf("Выполняю автоматическую проверку счётчиков...\n");
            bool ok = validate_field(field);
            if (!ok) {
                printf("Автоматическая проверка нашла несоответствия. Поле отброшено.\n");
                printf("Выберите: (R) перегенерировать, (P) новые параметры, (E) выйти\n");
                char o2;
                do { o2 = getchar(); } while (o2 == '\n' || o2 == ' ' || o2 == '\t');
                int j2; while ((j2 = getchar()) != '\n' && j2 != EOF) {}
                if (o2 == 'R' || o2 == 'r') { goto generate_again; }
                if (o2 == 'P' || o2 == 'p') { field_free(field); continue; }
                field_free(field); printf("Выход.\n"); return 0;
            }
            else {
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
                else {
                    int jj; while ((jj = getchar()) != '\n' && jj != EOF) {}
                    printf("Имя файла не введено — пропускаем сохранение.\n");
                }

                print_field_ascii(field, true);

                for (;;) {
                    printf("\nМеню после сохранения: 1) Показать поле  2) Сгенерировать новое поле  3) Выйти\nВыберите: ");
                    int cmd;
                    if (scanf("%d", &cmd) != 1) {
                        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
                        printf("Неверный ввод.\n");
                        continue;
                    }
                    if (cmd == 1) print_field_ascii(field, true);
                    else if (cmd == 2) { field_free(field); break; }
                    else if (cmd == 3) { field_free(field); printf("Выход.\n"); return 0; }
                    else printf("Неизвестная команда.\n");
                }
                continue;
            }
        }

        field_free(field);
    }

    return 0;
}
