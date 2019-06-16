#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <string>
#define WORLD_SIZE 300
#define ALIVE 0
#define BURNING 1
#define DEAD 2
#define EMPTY 3
#define RED al_map_rgb(255,0,0)
#define GREEN al_map_rgb(0,255,0)
#define BLACK al_map_rgb(0,0,0)
#define GREY al_map_rgb(156,156,156)
#define EMPTY_PROB 0.0001f
using namespace std;

void calcProb(double&nord,double&sud,double&ovest,double&est, int wind,double windForce){
switch (wind)
    {
    case 0:
        sud = 0.5f + windForce;
        nord = 0;
        est = 0.25f;
        ovest = 0.25f;
        break;
    case 1:
        sud = 0.5f + (windForce * 0.5f);
        nord = 0;
        ovest = 0.5f + (windForce * 0.5f);
        est = 0;
        break;
    case 2:
        sud = 0.25f;
        nord = 0.25f;
        est = 0;
        ovest = 0.5f + windForce;
        break;
    case 3:
        nord = 0.5f + (windForce * 0.5f);
        sud = 0;
        ovest = 0.5f + (windForce * 0.5f);
        est = 0;
        break;
    case 4:
        nord = 0.5f + windForce;
        sud = 0;
        est = 0.25f;
        ovest = 0.25f;
        break;
    case 5:
        nord = 0.5f + (windForce * 0.5f);
        sud = 0;
        est = 0.5f + (windForce * 0.5f);
        ovest = 0;
        break;
    case 6:
        est = 0.5f + windForce;
        ovest = 0;
        nord = 0.25f;
        sud = 0.25f;
        break;
    case 7:
        sud = 0.5f + (windForce * 0.5f);
        ovest= 0;
        est = 0.5f + (windForce * 0.5f);
        nord = 0;
        break;
    default:
        break;
    }
}

void computeNeigh(int myWorld[][WORLD_SIZE], int newWorld[][WORLD_SIZE], int upper[], int below[], int i, int j, int myRows, int wind, int windForce)
{
    double nord, sud, ovest, est, prob;
    calcProb(nord,sud,ovest,est,wind,windForce);
    if (i == 0)
    {
        if ((upper[j] == BURNING && ((double)rand())/RAND_MAX <= nord) || (myWorld[i + 1][j] == BURNING && ((double)rand())/RAND_MAX <= sud))
            newWorld[i][j] = BURNING;
    }
    else if (i == myRows - 1)
    {
        if ((below[j] == BURNING && ((double)rand())/RAND_MAX <= sud) || (myWorld[i - 1][j] == BURNING && ((double)rand())/RAND_MAX <= nord))
            newWorld[i][j] = BURNING;
    }
    else
    {
        if ((myWorld[i - 1][j] == BURNING && ((double)rand())/RAND_MAX <=nord) || (myWorld[i + 1][j] == BURNING && ((double)rand())/RAND_MAX <= sud))
            newWorld[i][j] = BURNING;
    }

    if (j == 0)
    {
        if (myWorld[i][j + 1] == BURNING && ((double)rand())/RAND_MAX <= est)
            newWorld[i][j] = BURNING;
    }
    else if (j == WORLD_SIZE - 1)
    {
        if (myWorld[i][j - 1] == BURNING && ((double)rand())/RAND_MAX <= ovest)
            newWorld[i][j] = BURNING;
    }
    else
    {
        if ((myWorld[i][j - 1] == BURNING && ((double)rand())/RAND_MAX <= ovest)|| (myWorld[i][j + 1] == BURNING && ((double)rand())/RAND_MAX <= est))
        {
            newWorld[i][j] = BURNING;
        }
    }
}
void sendHalo(int myRank, int commSize, int myRows, int myWorld[][WORLD_SIZE], int upper[], int below[])
{
    MPI_Status st;
    if (myRank == 0)
    {
        MPI_Bsend(myWorld[myRows - 1], WORLD_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Bsend(myWorld[0], WORLD_SIZE, MPI_INT, commSize - 1, 0, MPI_COMM_WORLD);
        MPI_Recv(upper, WORLD_SIZE, MPI_INT, commSize - 1, 0, MPI_COMM_WORLD, &st);
        MPI_Recv(below, WORLD_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &st);
    }
    else if (myRank == commSize - 1)
    {
        MPI_Bsend(myWorld[myRows - 1], WORLD_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Bsend(myWorld[0], WORLD_SIZE, MPI_INT, myRank - 1, 0, MPI_COMM_WORLD);
        MPI_Recv(upper, WORLD_SIZE, MPI_INT, myRank - 1, 0, MPI_COMM_WORLD, &st);
        MPI_Recv(below, WORLD_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
    }
    else
    {
        MPI_Bsend(myWorld[myRows - 1], WORLD_SIZE, MPI_INT, myRank + 1, 0, MPI_COMM_WORLD);
        MPI_Bsend(myWorld[0], WORLD_SIZE, MPI_INT, myRank - 1, 0, MPI_COMM_WORLD);
        MPI_Recv(upper, WORLD_SIZE, MPI_INT, myRank - 1, 0, MPI_COMM_WORLD, &st);
        MPI_Recv(below, WORLD_SIZE, MPI_INT, myRank + 1, 0, MPI_COMM_WORLD, &st);
    }
}
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int myRank, commSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    const int myRows = WORLD_SIZE / commSize;
    srand(time(0));
    int world[WORLD_SIZE][WORLD_SIZE];
    int newWorld[myRows][WORLD_SIZE];
    int oxygenWorld[myRows][WORLD_SIZE];
    string windsLow[8]={
        "./Up1.png",
        "./NE1.png",
        "./Right1.png",
        "./SE1.png",
        "./Up1.png",
        "./SE1.png",
        "./Right1.png",
        "./NE1.png"
    };
    string windsHigh[8] ={
        "./Up2.png",
        "./NE2.png",
        "./Right2.png",
        "./SE2.png",
        "./Up2.png",
        "./SE2.png",
        "./Right2.png",
        "./NE2.png"
    };
    ALLEGRO_DISPLAY* display;
    ALLEGRO_BITMAP * windBitmap;
    ALLEGRO_BITMAP * alive;
    ALLEGRO_BITMAP * dead;
    ALLEGRO_BITMAP * empty;
    ALLEGRO_BITMAP * burning;
    int myWorld[myRows][WORLD_SIZE];
    int windDirection;
    double windForce;
    int upper[WORLD_SIZE];
    int below[WORLD_SIZE];
    if (myRank == 0)
    {
        al_init();
        al_init_image_addon();
        display = al_create_display(1200,1200);
        alive = al_create_bitmap(4,4);
        al_set_target_bitmap(alive);
        al_clear_to_color(GREEN);
        dead = al_create_bitmap(4,4);
        al_set_target_bitmap(dead);
        al_clear_to_color(BLACK);
        burning = al_create_bitmap(4,4);
        al_set_target_bitmap(burning);
        al_clear_to_color(RED);
        empty = al_create_bitmap(4,4);
        al_set_target_bitmap(empty);
        al_clear_to_color(GREY);
        al_set_target_bitmap(al_get_backbuffer(display));
        for (int i = 0; i < WORLD_SIZE; i++)
        {
            for (int j = 0; j < WORLD_SIZE; j++)
            {
                if((double)rand()/RAND_MAX <= EMPTY_PROB)
                    world[i][j] = EMPTY;
                else
                    world[i][j] = ALIVE;
            }
        }
        world[WORLD_SIZE / 2][WORLD_SIZE / 2 - 1] = BURNING;
        world[100][40] = BURNING;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Scatter(world, WORLD_SIZE * (WORLD_SIZE / commSize), MPI_INT, myWorld, WORLD_SIZE * (WORLD_SIZE / commSize), MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 0; i < myRows; i++)
    {
        for (int j = 0; j < WORLD_SIZE; j++)
        {
            newWorld[i][j] = myWorld[i][j];
            oxygenWorld[i][j] = 100;
        }
    }
    for (int n = 0; n < 900; n++)
    {
        MPI_Status st;
        if (myRank == 0 && n%20==0)
        {
            windDirection = rand()%8;
            windForce = ((double)rand()) / RAND_MAX;
        }
        sendHalo(myRank, commSize, myRows, myWorld, upper, below);
        MPI_Bcast(&windDirection,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&windForce,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        #pragma omp parrallel for
        for (int i = 0; i < myRows; i++)
        {
            for (int j = 0; j < WORLD_SIZE; j++)
            {
                if (myWorld[i][j] == ALIVE)
                    computeNeigh(myWorld, newWorld, upper, below, i, j, myRows,windDirection,windForce);
            }
        }

        for (int i = 0; i < myRows; i++)
        {
            for (int j = 0; j < WORLD_SIZE; j++)
            {
                if (myWorld[i][j] == BURNING && oxygenWorld[i][j] == 0)
                {
                    newWorld[i][j] = DEAD;
                }else if(myWorld[i][j] == BURNING){
                    newWorld[i][j] = BURNING;
                    oxygenWorld[i][j] -=25;
                }
                myWorld[i][j] = newWorld[i][j];
            }
        }
        MPI_Gather(myWorld, WORLD_SIZE * (WORLD_SIZE / commSize), MPI_INT, world, WORLD_SIZE * (WORLD_SIZE / commSize), MPI_INT, 0, MPI_COMM_WORLD);
        if (myRank == 0)
        {
            int flip = 0;
            if(windForce > 0.5f){
                windBitmap = al_load_bitmap(windsHigh[windDirection].c_str());
            }else
            {
                windBitmap = al_load_bitmap(windsLow[windDirection].c_str());
            }
            al_clear_to_color(GREEN);
            if(windDirection==7||windDirection==6||windDirection==5)
                flip = ALLEGRO_FLIP_HORIZONTAL;
            if(windDirection == 4)
                flip = ALLEGRO_FLIP_VERTICAL;
            for (size_t i = 0; i < WORLD_SIZE; i++)
            {
                for (size_t j = 0; j < WORLD_SIZE; j++)
                {
                    if(world[i][j] == 1){
                        al_draw_bitmap(burning,j*4,i*4,0);
                    }
                    if(world[i][j] == 2){
                        al_draw_bitmap(dead,j*4,i*4,0);
                    }
                    if(world[i][j] == 3){
                        al_draw_bitmap(empty,j*4,i*4,0);
                    }
                }
            }
            al_draw_bitmap(windBitmap,750,50,flip);
            al_flip_display();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
