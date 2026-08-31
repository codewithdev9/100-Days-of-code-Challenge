#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_WORD_LEN 50
#define MAX_TRIES 6

const char *word_list[] = {
    "programming", "hangman", "computer", "developer",
    "keyboard", "function", "variable", "algorithm",
    "compiler", "structure"
};
#define WORD_COUNT (sizeof(word_list) / sizeof(word_list[0]))

void display_hangman(int wrong_guesses) {
    const char *stages[] = {
        "\n  +---+\n      |\n      |\n      |\n     ===\n",
        "\n  +---+\n  O   |\n      |\n      |\n     ===\n",
        "\n  +---+\n  O   |\n  |   |\n      |\n     ===\n",
        "\n  +---+\n  O   |\n /|   |\n      |\n     ===\n",
        "\n  +---+\n  O   |\n /|\\  |\n      |\n     ===\n",
        "\n  +---+\n  O   |\n /|\\  |\n /    |\n     ===\n",
        "\n  +---+\n  O   |\n /|\\  |\n / \\  |\n     ===\n"
    };
    printf("%s\n", stages[wrong_guesses]);
}

void display_word(const char *word, int *guessed, int word_len) {
    printf("Word: ");
    for (int i = 0; i < word_len; i++) {
        if (guessed[i])
            printf("%c ", word[i]);
        else
            printf("_ ");
    }
    printf("\n");
}

int already_guessed(char guess, char *guessed_letters, int count) {
    for (int i = 0; i < count; i++) {
        if (guessed_letters[i] == guess)
            return 1;
    }
    return 0;
}

int main() {
    srand((unsigned int)time(NULL));

    char word[MAX_WORD_LEN];
    strcpy(word, word_list[rand() % WORD_COUNT]);
    int word_len = strlen(word);

    int guessed[MAX_WORD_LEN] = {0};
    char guessed_letters[26];
    int guessed_count = 0;

    int wrong_guesses = 0;
    int correct_count = 0;

    printf("=====================================\n");
    printf("       WELCOME TO HANGMAN GAME       \n");
    printf("=====================================\n");
    printf("Guess the word, letter by letter.\n");
    printf("You have %d wrong tries allowed.\n\n", MAX_TRIES);

    while (wrong_guesses < MAX_TRIES && correct_count < word_len) {
        display_hangman(wrong_guesses);
        display_word(word, guessed, word_len);

        printf("Guessed letters: ");
        for (int i = 0; i < guessed_count; i++)
            printf("%c ", guessed_letters[i]);
        printf("\n");

        printf("Enter a letter: ");
        char guess;
        scanf(" %c", &guess);
        guess = tolower(guess);

        if (!isalpha(guess)) {
            printf("Please enter a valid alphabet letter.\n\n");
            continue;
        }

        if (already_guessed(guess, guessed_letters, guessed_count)) {
            printf("You already guessed '%c'. Try another.\n\n", guess);
            continue;
        }

        guessed_letters[guessed_count++] = guess;

        int found = 0;
        for (int i = 0; i < word_len; i++) {
            if (word[i] == guess && !guessed[i]) {
                guessed[i] = 1;
                found = 1;
                correct_count++;
            }
        }

        if (found) {
            printf("Good guess! '%c' is in the word.\n\n", guess);
        } else {
            wrong_guesses++;
            printf("Wrong guess! '%c' is not in the word.\n\n", guess);
        }
    }

    display_hangman(wrong_guesses);

    if (correct_count == word_len) {
        printf("Congratulations! You guessed the word: %s\n", word);
    } else {
        printf("Game Over! The correct word was: %s\n", word);
    }

    return 0;
}