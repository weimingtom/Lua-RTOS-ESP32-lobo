#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "unity.h"
#include "rom/ets_sys.h"
#include "rom/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define unity_printf ets_printf

// Pointers to the head and tail of linked list of test description structs:
static struct test_desc_t* s_unity_tests_first = NULL;
static struct test_desc_t* s_unity_tests_last = NULL;


void unity_putc(int c)
{
    if (c == '\n') 
    {
        uart_tx_one_char('\n');
        uart_tx_one_char('\r');
    }
    else if (c == '\r') 
    {
    }
    else 
    {
        uart_tx_one_char(c);
    }
}

void unity_flush()
{
    uart_tx_wait_idle(0);   // assume that output goes to UART0
}

void unity_testcase_register(struct test_desc_t* desc)
{
    if (!s_unity_tests_first) 
    {
        s_unity_tests_first = desc;
    }
    else 
    {
        s_unity_tests_last->next = desc;
    }
    s_unity_tests_last = desc;
    desc->next = NULL;
}

static void unity_run_single_test(const struct test_desc_t* test)
{
    Unity.TestFile = test->file;
    Unity.CurrentDetail1 = test->desc;
    UnityDefaultTestRun(test->fn, test->name, test->line);
}

static void unity_run_single_test_by_index(int index)
{
    const struct test_desc_t* test;
    for (test = s_unity_tests_first; test != NULL && index != 0; test = test->next, --index)
    {
    }
    if (test != NULL)
    {
        unity_run_single_test(test);
    }
    
}

static void unity_run_single_test_by_name(const char* filter)
{  
    char tmp[256];
    strncpy(tmp, filter + 1, sizeof(tmp) - 1);
    tmp[strlen(filter) - 2] = 0;
    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        if (strstr(test->name, tmp) != NULL)
        {
            unity_run_single_test(test);
        }
    }    
}

void unity_run_all_tests()
{
    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        unity_run_single_test(test);
    }
}

void unity_run_tests_with_filter(const char* filter)
{
    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        if (strstr(test->desc, filter) != NULL)
        {
            unity_run_single_test(test);
        }
    }
}

static void trim_trailing_space(char* str)
{
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace((int) *end))
    {
        *end = 0;
        --end;
    }
}

static int print_test_menu(void)
{
    int test_counter = 0;
    unity_printf("\n\nHere's the test menu, pick your combo:\n");
    for (const struct test_desc_t* test = s_unity_tests_first;
         test != NULL;
         test = test->next, ++test_counter)
    {
        unity_printf("(%d)\t\"%s\" %s\n", test_counter + 1, test->name, test->desc);
    }
    return test_counter;
}

void unity_run_menu()
{
    int test_count = print_test_menu();
    while (true)
    {
        char cmdline[256] = { 0 };
        while(strlen(cmdline) == 0)
        {
            /* Flush anything already in the RX buffer */
            while(uart_rx_one_char((uint8_t *) cmdline) == OK) {
            }
            /* Read input */
            UartRxString((uint8_t*) cmdline, sizeof(cmdline) - 1);
            trim_trailing_space(cmdline);
            if(strlen(cmdline) == 0) {
                /* if input was newline, print a new menu */
                print_test_menu();
            }
        }

        UNITY_BEGIN();

        if (cmdline[0] == '*')
        {
            unity_run_all_tests();
        }
        else if (cmdline[0] =='[')
        {
            unity_run_tests_with_filter(cmdline);
        }
        else if (cmdline[0] =='"')
        {
            unity_run_single_test_by_name(cmdline);
        }
        else
        {
            int test_index = strtol(cmdline, NULL, 10);
            if (test_index >= 1 && test_index <= test_count)
            {
                uint32_t start = esp_log_timestamp(); /* hacky way to get ms */
                unity_run_single_test_by_index(test_index - 1);
                uint32_t end = esp_log_timestamp();
                printf("Test ran in %dms\n", end - start);
            }
        }

        UNITY_END();

        printf("Enter next test, or 'enter' to see menu\n");
    }
}

