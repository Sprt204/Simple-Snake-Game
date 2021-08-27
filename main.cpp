#include <iostream>
#include <vector>
#include <random>
#include <functional>
#include <binders.h>

#include <SDL.h>

const int FPS = 15;
const Uint32 frameDelay = 1000 / FPS;

const char* name = "Snake";
int width = 640;
int height = 480;
int posX = SDL_WINDOWPOS_CENTERED;
int posY = SDL_WINDOWPOS_CENTERED;

bool running = true;

const int BLOCK_SIZE = 20;
const int MIDDLE_POSX = width / (BLOCK_SIZE * 2);
const int MIDDLE_POSY = height / (BLOCK_SIZE * 2);
const int WIDTH_LIMIT = width / BLOCK_SIZE;
const int HEIGHT_LIMIT = height / BLOCK_SIZE;

const int INIT_SNAKE_SIZE = 3;

SDL_Window* window = NULL;
SDL_Renderer* render = NULL;

std::default_random_engine generator;
std::uniform_int_distribution<int> Xdistribution(0, WIDTH_LIMIT);
std::uniform_int_distribution<int> Ydistribution(0, HEIGHT_LIMIT);

auto genRandomX = std::bind(Xdistribution, generator);
auto genRandomY = std::bind(Ydistribution, generator);

int score = 0;

enum Direction
{
    NONE = 0,
    UP, RIGHT, DOWN, LEFT,
};

void processKeyDown(const SDL_Event&);
bool isOppositeDirection(const Direction, const Direction);

class Size
{
    public:
        Size(){}
        Size(int width, int height)
        {
            this->w = width;
            this->h = height;
        }

        int w = 0;
        int h = 0;
};

class Position
{
public:

    Position(){}
    Position(int x, int y)
    {
        this->x = x;
        this->y = y;
    }

    bool operator==(Position pos)
    {
        return (this->x == pos.x && this->y == pos.y);
    }

    int x, y;
};

class Color
{
public:
    Color(){}
    Color(Uint8 r, Uint8 g, Uint8 b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }

    Uint8 r, g, b;
};

class Snake
{
public:
    Snake(){}
    Snake(Position pos, Color c, Size s)
        : color(c), _size(s)
    {
        body.push_back(pos);
        length++;
    }

    void onFrame()
    {
        draw();
        if(currentDirection != NONE)
        {
            updateBody();
            moveHead();
            checkCollisionOnBody();
            checkForMapLimits();
        }
    }

    void add()
    {
        auto& lastPart = body[length - 1];

        body.push_back(lastPart);
        length++;
    }

    void updateDirection(Direction d)
    {
        this->lastDirection = this->currentDirection;
        this->currentDirection = d;
    }

    Position getHeadPosition()
    {
        auto& head = body[0];
        return head;
    }

private:
    void draw() const
    {
        for(long int i = 0; i < length; i++)
        {
            auto& pos = body[i];

            SDL_Rect tempRect = {
                .x = pos.x * _size.w,
                .y = pos.y * _size.h,
                .w = _size.w,
                .h = _size.h
            };

            SDL_SetRenderDrawColor(render, color.r, color.g, color.b, 255);
            SDL_RenderFillRect(render, &tempRect);
        }
    }

    void updateBody()
    {
        for(long int i = length-1; i > 0; i--)
        {
            auto& nextPosition = body[i-1];
            auto& currentPosition = body[i];

            currentPosition = nextPosition;
        }
    }

    void moveHead()
    {
        auto& head = body[0];
        auto&& allow = !isOppositeDirection(currentDirection, lastDirection);

        if(allow)
        {
            switch(currentDirection)
            {
            case UP:
                head.y -= 1;
                break;

            case RIGHT:
                head.x += 1;
                break;

            case DOWN:
                head.y += 1;
                break;

            case LEFT:
                head.x -= 1;
                break;

            case NONE:
                break;
            }
        }
        else
        {
            currentDirection = lastDirection;
            lastDirection = NONE;
            moveHead();
        }
    }

    void checkCollisionOnBody() const
    {
        auto& head = body[0];
        for(long int i = length - 1; i > 1; i--)
        {
            auto& currentPartOfBody = body[i];
            if(currentPartOfBody.x == head.x && currentPartOfBody.y == head.y)
                running = false;
        }
    }

    void checkForMapLimits()
    {
        auto& head = body[0];

        if(head.x < 0 || head.x > WIDTH_LIMIT || head.y < 0 || head.y > HEIGHT_LIMIT)
            running = false;
    }

private:
    std::vector<Position> body;
    Color color;
    Size _size;

    long int length = 0;

    Direction currentDirection = NONE;
    Direction lastDirection = NONE;
};

class Apple
{
public:
    Apple(){}
    Apple(Position pos, Color c, Size s)
        : position(pos), color(c), _size(s)
    {}

    void onFrame()
    {
        draw();
    }

    Position updatePosition()
    {
        position.x = genRandomX();
        position.y = genRandomY();

        return position;
    }

private:
    void draw()
    {
        SDL_Rect tempRect = {
            .x = position.x * _size.w,
            .y = position.y * _size.h,
            .w = _size.w,
            .h = _size.h,
        };

        SDL_SetRenderDrawColor(render, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(render, &tempRect);
    }

private:
    Position position;
    Color color;
    Size _size;
};

Snake snake(Position(MIDDLE_POSX, MIDDLE_POSY), Color(0,255,0), Size(BLOCK_SIZE, BLOCK_SIZE));
Apple apple(Position(genRandomX(), genRandomY()), Color(255,0,0), Size(BLOCK_SIZE, BLOCK_SIZE));

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(name, posX, posY, width, height, 0);

    render = SDL_CreateRenderer(window, -1, 0);

    SDL_Event event;

    Uint32 frameStart = 0, frameTime = 0;

    for(int i = 0; i < INIT_SNAKE_SIZE; i++)
        snake.add();

    auto applePos = apple.updatePosition();

    while(running)
    {
        frameStart = SDL_GetTicks();

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                processKeyDown(event);
                break;
            }
        }

        SDL_SetRenderDrawColor(render, 0, 0, 32, 255);
        SDL_RenderClear(render);

        snake.onFrame();
        apple.onFrame();

        SDL_RenderPresent(render);

        auto&& snakeHead = snake.getHeadPosition();
        if(snakeHead == applePos)
        {
            snake.add();
            applePos = apple.updatePosition();
            score++;
        }

        frameTime = SDL_GetTicks() - frameStart;
        if(frameDelay > frameTime)
            SDL_Delay(frameDelay - frameTime);
    }

    SDL_Quit();

    std::cout << "Final Score: " << score << std::endl;

    return 0;
}

void processKeyDown(const SDL_Event& event)
{
    auto& key = event.key.keysym.sym;

    switch(key)
    {
    case SDLK_w:
        snake.updateDirection(Direction::UP);
        break;

    case SDLK_d:
        snake.updateDirection(Direction::RIGHT);
        break;

    case SDLK_s:
        snake.updateDirection(Direction::DOWN);
        break;

    case SDLK_a:
        snake.updateDirection(Direction::LEFT);
        break;

    case SDLK_ESCAPE:
        snake.updateDirection(Direction::NONE);
        break;
    }
}

bool isOppositeDirection(const Direction d1, const Direction d2)
{
    bool result = false;
    if( (d1 == RIGHT && d2 == LEFT) || (d1 == LEFT && d2 == RIGHT) )
        result = true;
    else if( (d1 == UP && d2 == DOWN) || (d1 == DOWN && d2 == UP) )
        result = true;

    return result;
}
