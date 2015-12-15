/**
 * The tester class for ubler. Just reads in input from the terminal and runs
 * create_and_run_requests, then checks the output stats.
 */

#include "ubler.h"
#include "ubler_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

bool check_equal(const char *msg, int got, int expected)
{
    if (expected != got)
    {
        printf("Error: %s mismatch, got %d, expected %d\n", msg, got, expected);
        return true;
    }
    return false;
}

/**
 * Looks for command line options num_requests num_drivers num_restaurants, and optionally
 *   --mix-up-meals
 * Starts up ubler with num_requests at num_restaurants, serviced by num_drivers
 * If --mix-up-meals is passed, mixes up mapping from meals to customers
 *
 * Also looks for a conf file with sleepTime and deliverMealsAndCustomers
 * sleepTime is number of microseconds to sleep while driving
 * If deliverMealsAndCustomers is true, the dispatcher can return both meals and
 *   customers; otherwise, it only returns meals
 * Expected format is as follows:
 *
 * sleepTime:10,deliverMealsAndCustomers:1
 */

const char *confOptions[2] = {
    "sleepTime",
    "deliverMealsAndCustomers"
};

#define MAX_CONF_FILE_SIZE 4096

int main(int num_args, char *argv[])
{
    int num_requests = 10;
    int num_drivers = 1;
    int num_restaurants = 5;
    struct ubler_stats stats = {0,0,0,0,0,0,0,0,0,0};
    struct ubler_params params = {10, true, false};

    if (num_args == 1)
    {
        printf("Usage: ubler_test num_requests num_drivers num_restaurants [--mix-up-meals]\n");
        printf("Using defaults: 10 requests, 1 driver, 5 restaurants, no mix up\n\n");
    }
    else if(num_args >= 4 && num_args <= 5)
    {
        num_requests = atoi(argv[1]);
        num_drivers = atoi(argv[2]);
        num_restaurants = atoi(argv[3]);
        if (num_args == 5 &&
            strncmp("--mix-up-meals", argv[4], strlen("--mix-up-meals")) == 0)
        {
            params.mixUpMeals = true;
        }
    }
    else
    {
        printf("Usage: ubler_test num_requests num_drivers num_restaurants\n");
        return 0;
    }

    FILE *conf = fopen("ubler.conf", "r");
    if (conf != NULL)
    {
        struct stat file_stats;
        if (stat("ubler.conf", &file_stats) != 0)
        {
            printf("Could not determine size of conf file\n");
            return 1;
        }
        if (file_stats.st_size > MAX_CONF_FILE_SIZE)
        {
            printf("Conf file too long\n");
            return 1;
        }

        char *confPayload = malloc(MAX_CONF_FILE_SIZE);
        memset(confPayload, 0, MAX_CONF_FILE_SIZE);
        fscanf(conf, "%s", confPayload);

        char *curOption = confPayload;
        int optionVal = 0;

        while (*curOption != 0)
        {
            // find comma
            char *colon = strchr(curOption, ':');
            if (!colon)
            {
                break;
            }
            *colon = 0;
            char *comma = strchr(colon + 1, ',');
            if (comma)
            {
                *comma = 0;
            }
            optionVal = atoi(colon + 1);
            if (strcmp(curOption, "sleepTime") == 0)
            {
                params.sleepTime = optionVal;
            }
            if (strcmp(curOption, "deliverMealsAndCustomers") == 0)
            {
                params.deliverMealsAndCustomers = optionVal;
            }
            if (comma)
            {
                curOption = comma + 1;
            }
        }

        free(confPayload);
    }
    int result = create_and_run_requests(num_requests, num_restaurants, num_drivers, &params, &stats);
    if (result > 0)
    {
        printf("Parameters too high\n");
        return 1;
    }

    bool error = false;

    error = check_equal("Requests picked up", stats.total_meals_picked_up + stats.total_customers_picked_up, num_requests) || error;
    error = check_equal("Requests ignored", stats.total_meals_ignored + stats.total_customers_ignored, 0) || error;
    error = check_equal("Requests overserviced", stats.total_meals_overserviced + stats.total_customers_overserviced, 0) || error;
    error = check_equal("Driver pickups, requests", stats.driver_meals_picked_up + stats.driver_customers_picked_up, num_requests) || error;
    error = check_equal("Driver deliveries, requests", stats.driver_meals_delivered + stats.driver_customers_delivered, num_requests) || error;
    error = check_equal("Driver count and request counts",
        stats.driver_meals_picked_up + stats.driver_customers_picked_up,
        stats.total_meals_picked_up + stats.total_customers_picked_up) || error;

    if (!error)
    {
        printf("Success!\n");
    }
    return 0;
}
