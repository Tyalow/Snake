//+X is down, +Z is LEFT

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <random>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <custom/camera.h>
#include <custom/shader.h>


enum SnakeDirection
{
    MOVING_UP,
    MOVING_DOWN,
    MOVING_LEFT,
    MOVING_RIGHT
};

struct SnakeSegment
{
    std::pair<float, float> frontCoord{};
    std::pair<float, float> backCoord{};
    SnakeDirection direction{};
};

struct Snake
{
    std::vector<SnakeSegment> snakeBody{};
    SnakeDirection currentDirection{};
    float length{ 1.0f };
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Snake& snake);
void initializeProgram();
bool initializeWindow(const unsigned int width, const unsigned int height, GLFWwindow* window);
void initVertexObjects(unsigned int& VBO, unsigned int& VAO);
void drawPlatform(glm::mat4& model, Shader& ourShader);
void drawSnake(glm::mat4& model, Shader& ourShader, Snake& snake);
void drawFood(glm::mat4& model, Shader& ourShader, std::vector<std::pair<float, float>>& foodContainer);
void moveSnake(Snake& snake);
void handleMovement(Snake& snake, bool moveBack);
float getSnakeLength(Snake& snake);
void addSegment(Snake& snake);
float getSegmentLength(SnakeSegment& snakeSegment, bool inX);
void handleCollisions(GLFWwindow* window, Snake& snake, std::vector<std::pair<float, float>>& foodContainer);
bool checkCollision(SnakeSegment& frontSegment, SnakeSegment& segment);
bool inBox(float x1, float x2, float z1, float z2, std::pair<float, float>& point);
bool checkFoodCollision(SnakeSegment& segment, std::pair<float, float>& foodCoords);
void addFood(Snake& snake, std::vector<std::pair<float,float>>& foodContainer);
void setBoundsFromSegment(float& x1, float& x2, float& z1, float& z2, SnakeSegment& segment);

//Settings
const unsigned int SCR_WIDTH{ 800 };
const unsigned int SCR_HEIGHT{ 600 };

//Camera/Mouse
glm::vec3 camPos{ glm::vec3(0.0f, 6.0f, 0.0f) };
glm::vec3 camUp{ glm::vec3(0.0f, 1.0f, 0.0f) };
float pitch{ -90.0f };
float yaw{ -90.0f };
Camera camera{ camPos, camUp, 0.0f, -90.0f };

//Timing variables
float deltaTime{ 0.0f }; // Time between current frame and last frame
float lastFrame{ 0.0f }; // Time of last frame

//Shader uniform
glm::vec3 platformColor{ glm::vec3(0.3f, 0.3f, 0.3f) };
glm::vec3 snakeColor{ glm::vec3(1.0f, 1.0f, 0.0f) };
glm::vec3 foodColor{ glm::vec3(1.0f, 1.0f, 1.0f) };

//Platform variables
glm::vec3 platformPosition{ glm::vec3(0.0f, -1.0f, 0.0f) };
const float platformScale = 5.0f;

//Snake variables
const float snakeMovespeed = 1.0f;
const float snakeRadius = 0.125;

int main()
{
    initializeProgram();

    GLFWwindow* window{ glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL) };
    if (!(initializeWindow(SCR_HEIGHT, SCR_HEIGHT, window)))
        return -1;

    Shader ourShader("Resources/shader.vs", "Resources/shader.fs");

    //Generate vertex buffer object, and connect vertices to it
    unsigned int VBO, VAO;
    initVertexObjects(VBO, VAO);

    ourShader.use();

    //Enable depth testing and hide cursor + capture mouse
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glm::mat4 model{};
    glm::mat4 view{};
    glm::mat4 projection{};

    //Init snake
    Snake snake{};
    snake.snakeBody.push_back(SnakeSegment{ {0.0f, 0.0f}, {0.5f, 0.0f}, MOVING_UP });
    snake.currentDirection = MOVING_UP;

    //Init food container
    std::vector<std::pair<float, float>> foodContainer;

    int loopCount{ 0 };
    while (!glfwWindowShouldClose(window))
    {
        //Input
        processInput(window, snake);

        //Timing 
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //Rendering commands here, have to clear color and depth buffers before each drawing pass
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create MVP matrices and send to shader 
        view = camera.getViewMatrix();
        ourShader.setMat4("view", view);

        projection = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);
  
        glBindVertexArray(VAO);
        //Draw calls for platform, snake and food
        drawPlatform(model, ourShader);
        drawSnake(model, ourShader, snake);
        drawFood(model, ourShader, foodContainer);

        if (loopCount % 125 == 0)
        {
            addFood(snake, foodContainer);
            loopCount = 0;
        }
        ++loopCount;

        moveSnake(snake);
        handleCollisions(window, snake, foodContainer);

        //Check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, Snake& snake)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        snake.currentDirection = MOVING_UP;
    }    
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        snake.currentDirection = MOVING_DOWN;
    }       
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        snake.currentDirection = MOVING_LEFT;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        snake.currentDirection = MOVING_RIGHT;
    }
}

void initializeProgram()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

bool initializeWindow(const unsigned int width, const unsigned int height, GLFWwindow* window)
{
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    return true;
}

void initVertexObjects(unsigned int& VBO, unsigned int& VAO)
{
    float vertices[] = {
    -0.5f, -0.5f, -0.5f, 
     0.5f, -0.5f, -0.5f, 
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,

    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,

     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,

    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    //Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //Note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Unbind
    glBindVertexArray(0);
}

void drawPlatform(glm::mat4& model, Shader& ourShader)
{
    ourShader.setVec3("boxColor", platformColor);
    model = glm::translate(glm::mat4(1.0f), platformPosition);
    model = glm::scale(model, glm::vec3(platformScale, 0.5f, platformScale));
    ourShader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawSnake(glm::mat4& model, Shader& ourShader, Snake& snake)
{
    ourShader.setVec3("boxColor", snakeColor);
    for (auto &segment : snake.snakeBody)
    {
        glm::vec3 segmentPosition{};
        model = glm::mat4(1.0f);
        if (segment.direction == MOVING_UP || segment.direction == MOVING_DOWN)
        {
            float xScale{ std::abs(segment.frontCoord.first - segment.backCoord.first) };
            segmentPosition.x = (segment.frontCoord.first + segment.backCoord.first)/2;
            segmentPosition.y = 0.5f;
            segmentPosition.z = segment.frontCoord.second;
            model = glm::translate(model, segmentPosition);
            model = glm::scale(model, glm::vec3(xScale, 0.25f, 0.25f));
        }
        else if (segment.direction == MOVING_LEFT || segment.direction == MOVING_RIGHT)
        {
            float zScale{ std::abs(segment.frontCoord.second - segment.backCoord.second) };
            segmentPosition.x = segment.frontCoord.first;
            segmentPosition.y = 0.5f;
            segmentPosition.z = (segment.frontCoord.second + segment.backCoord.second) / 2;
            model = glm::translate(model, segmentPosition);
            model = glm::scale(model, glm::vec3(0.25f, 0.25f, zScale));
        }
        ourShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void drawFood(glm::mat4& model, Shader& ourShader, std::vector<std::pair<float, float>>& foodContainer)
{
    for (auto& foodPiece : foodContainer)
    {
        ourShader.setVec3("boxColor", foodColor);
        model = glm::translate(glm::mat4(1.0f), glm::vec3{foodPiece.first, 0.5f, foodPiece.second});
        model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
        ourShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void moveSnake(Snake& snake)
{
    float snakeLength{ getSnakeLength(snake) };
    if (snake.currentDirection != snake.snakeBody[0].direction)
    {
        addSegment(snake);
        handleMovement(snake, !(snakeLength < snake.length));
    }
    else
    {
        handleMovement(snake, !(snakeLength < snake.length));
    }
}

void handleMovement(Snake& snake, bool moveBack)
{
    switch (snake.snakeBody[0].direction)
    {
        case MOVING_UP:
            snake.snakeBody[0].frontCoord.first -= snakeMovespeed * deltaTime;
            break;
        case MOVING_DOWN:
            snake.snakeBody[0].frontCoord.first += snakeMovespeed * deltaTime;
            break;
        case MOVING_LEFT:
            snake.snakeBody[0].frontCoord.second += snakeMovespeed * deltaTime;
            break;
        case MOVING_RIGHT:
            snake.snakeBody[0].frontCoord.second -= snakeMovespeed * deltaTime;
            break;
    }

    if (moveBack)
    {       
        std::size_t numSegments{ snake.snakeBody.size() - 1 };
        float distanceIncrement = snakeMovespeed * deltaTime;
        //Check if snake movement deletes a segment
        if (snake.snakeBody[numSegments].direction == MOVING_UP || snake.snakeBody[numSegments].direction == MOVING_DOWN)
        {
            float currentSegmentLength{ getSegmentLength(snake.snakeBody[numSegments], true) };
            if (distanceIncrement >= currentSegmentLength)
            {
                snake.snakeBody.pop_back();
                distanceIncrement -= currentSegmentLength;
            }
        }
        else
        {
            float currentSegmentLength{ getSegmentLength(snake.snakeBody[numSegments], false) };
            if (distanceIncrement >= currentSegmentLength)
            {
                snake.snakeBody.pop_back();
                distanceIncrement -= currentSegmentLength;
            }
        }

        numSegments = snake.snakeBody.size() - 1;
        switch (snake.snakeBody[numSegments].direction)
        {
            case MOVING_UP:
                snake.snakeBody[numSegments].backCoord.first -= distanceIncrement;
                break;
            case MOVING_DOWN:
                snake.snakeBody[numSegments].backCoord.first += distanceIncrement;
                break;
            case MOVING_LEFT:
                snake.snakeBody[numSegments].backCoord.second += distanceIncrement;
                break;
            case MOVING_RIGHT:
                snake.snakeBody[numSegments].backCoord.second -= distanceIncrement;
                break;
        } 
    }        
}

float getSnakeLength(Snake& snake)
{
    float totalLength{ 0 };
    for (auto& segment : snake.snakeBody)
    {
        if (segment.direction == MOVING_UP || segment.direction == MOVING_DOWN)
            totalLength += getSegmentLength(segment, true);
        else if (segment.direction == MOVING_LEFT || segment.direction == MOVING_RIGHT)
            totalLength += getSegmentLength(segment, false);
    }
    return totalLength;
}

float getSegmentLength(SnakeSegment& snakeSegment, bool inX)
{
    if(inX)
        return std::abs(snakeSegment.frontCoord.first - snakeSegment.backCoord.first);
    return std::abs(snakeSegment.frontCoord.second - snakeSegment.backCoord.second);
}

void addSegment(Snake& snake)
{
    std::pair<float, float> headCoord{ snake.snakeBody[0].frontCoord };  
    switch (snake.snakeBody[0].direction)
    { 
        case MOVING_UP:
            switch (snake.currentDirection)
            {
                case MOVING_LEFT:
                    snake.snakeBody[0].frontCoord.first += 2*snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first + snakeRadius, headCoord.second + snakeRadius},{headCoord.first + snakeRadius, headCoord.second - snakeRadius},MOVING_LEFT });
                    break;
                case MOVING_RIGHT:
                    snake.snakeBody[0].frontCoord.first += 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first + snakeRadius, headCoord.second - snakeRadius},{headCoord.first + snakeRadius, headCoord.second + snakeRadius},MOVING_RIGHT });
                    break;
                case MOVING_DOWN:
                    snake.currentDirection = snake.snakeBody[0].direction;
                    break;
            }
            break;
        case MOVING_DOWN:
            switch (snake.currentDirection)
            {
                case MOVING_LEFT:
                    snake.snakeBody[0].frontCoord.first -= 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first - snakeRadius, headCoord.second + snakeRadius},{headCoord.first - snakeRadius, headCoord.second - snakeRadius},MOVING_LEFT });
                    break;
                case MOVING_RIGHT:
                    snake.snakeBody[0].frontCoord.first -= 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first - snakeRadius, headCoord.second - snakeRadius},{headCoord.first - snakeRadius, headCoord.second + snakeRadius},MOVING_RIGHT });
                    break;
                case MOVING_UP:
                    snake.currentDirection = snake.snakeBody[0].direction;
                    break;
            }
            break;
        case MOVING_LEFT:
            switch (snake.currentDirection)
            {
                case MOVING_UP:
                    snake.snakeBody[0].frontCoord.second -= 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first - snakeRadius, headCoord.second - snakeRadius},{headCoord.first + snakeRadius, headCoord.second - snakeRadius},MOVING_UP });
                    break;
                case MOVING_DOWN:
                    snake.snakeBody[0].frontCoord.second -= 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first + snakeRadius, headCoord.second - snakeRadius},{headCoord.first - snakeRadius, headCoord.second - snakeRadius},MOVING_DOWN });
                    break;
                case MOVING_RIGHT:
                    snake.currentDirection = snake.snakeBody[0].direction;
                    break;
            }
            break;
        case MOVING_RIGHT:
            switch (snake.currentDirection)
            {
                case MOVING_UP:
                    snake.snakeBody[0].frontCoord.second += 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first - snakeRadius, headCoord.second + snakeRadius},{headCoord.first + snakeRadius, headCoord.second + snakeRadius},MOVING_UP });
                    break;
                case MOVING_DOWN:
                    snake.snakeBody[0].frontCoord.second += 2 * snakeRadius;
                    snake.snakeBody.insert(snake.snakeBody.begin(), SnakeSegment{ {headCoord.first + snakeRadius, headCoord.second + snakeRadius},{headCoord.first - snakeRadius, headCoord.second + snakeRadius},MOVING_DOWN });
                    break;
                case MOVING_LEFT:
                    snake.currentDirection = snake.snakeBody[0].direction;
                    break;
            }
            break;
    }
}

void handleCollisions(GLFWwindow* window, Snake& snake, std::vector<std::pair<float, float>>& foodContainer)
{
    //Platform Collision
    if (snake.snakeBody[0].frontCoord.first > (platformPosition.x + (platformScale * 0.5)) || snake.snakeBody[0].frontCoord.first < (platformPosition.x - (platformScale * 0.5)))
        glfwSetWindowShouldClose(window, true);
    if (snake.snakeBody[0].frontCoord.second > (platformPosition.z + (platformScale * 0.5)) || snake.snakeBody[0].frontCoord.second < (platformPosition.z - (platformScale * 0.5)))
        glfwSetWindowShouldClose(window, true);

    //Self Collision
    for (int i{ 2 }; i < snake.snakeBody.size(); i++)
    {
        if(checkCollision(snake.snakeBody[0], snake.snakeBody[i]))
            glfwSetWindowShouldClose(window, true);
    }

    //Food Collision
    for (int i{ 0 }; i < foodContainer.size(); i++)
    {
        if (checkFoodCollision(snake.snakeBody[0], foodContainer[i]))
        {
            snake.length += 2 * snakeRadius;
            foodContainer.erase(foodContainer.begin() + i);
            break;
        }       
    }
}

bool checkCollision(SnakeSegment& frontSegment, SnakeSegment& segment)
{
    float x1{};
    float x2{};
    float z1{};
    float z2{};
    setBoundsFromSegment(x1, x2, z1, z2, segment);

    if (frontSegment.direction == MOVING_DOWN || frontSegment.direction == MOVING_UP)
    {
        //Pair for each leading corner
        std::pair<float, float> pair1{ frontSegment.frontCoord.first, frontSegment.frontCoord.second + snakeRadius };
        std::pair<float, float> pair2{ frontSegment.frontCoord.first, frontSegment.frontCoord.second - snakeRadius };
        return(inBox(x1, x2, z1, z2, pair1) || inBox(x1, x2, z1, z2, pair2));
    }
    else
    {
        //Pair for each leading corner
        std::pair<float, float> pair1{ frontSegment.frontCoord.first + snakeRadius, frontSegment.frontCoord.second };
        std::pair<float, float> pair2{ frontSegment.frontCoord.first - snakeRadius, frontSegment.frontCoord.second };
        return(inBox(x1, x2, z1, z2, pair1) || inBox(x1, x2, z1, z2, pair2));
    }
}

bool inBox(float x1, float x2, float z1, float z2, std::pair<float, float>& point)
{
    return (((x1 < point.first) && (point.first < x2)) && ((z1 < point.second) && (point.second < z2)));
}

bool checkFoodCollision(SnakeSegment& frontSegment, std::pair<float,float>& foodCoords)
{
    float x1{ foodCoords.first - snakeRadius };
    float x2{ foodCoords.first + snakeRadius };
    float z1{ foodCoords.second - snakeRadius };
    float z2{ foodCoords.second + snakeRadius };
    if (frontSegment.direction == MOVING_DOWN || frontSegment.direction == MOVING_UP)
    {
        //Pair for each leading corner
        std::pair<float, float> pair1{ frontSegment.frontCoord.first, frontSegment.frontCoord.second + snakeRadius };
        std::pair<float, float> pair2{ frontSegment.frontCoord.first, frontSegment.frontCoord.second - snakeRadius };
        return(inBox(x1, x2, z1, z2, pair1) || inBox(x1, x2, z1, z2, pair2));
    }
    else
    {
        //Pair for each leading corner
        std::pair<float, float> pair1{ frontSegment.frontCoord.first + snakeRadius, frontSegment.frontCoord.second };
        std::pair<float, float> pair2{ frontSegment.frontCoord.first - snakeRadius, frontSegment.frontCoord.second };
        return(inBox(x1, x2, z1, z2, pair1) || inBox(x1, x2, z1, z2, pair2));
    }
}

void addFood(Snake& snake, std::vector<std::pair<float, float>>& foodContainer)
{
    unsigned int seed{ static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()) };
    std::minstd_rand randomGen{ seed };
    float sample{ static_cast<float>(randomGen()) };
    sample = (platformScale * (sample / randomGen.max()) - (platformScale / 2)) * ((platformScale - (2 * snakeRadius)) / (platformScale));
    float xCoord{ sample };
    sample = static_cast<float>(randomGen());
    sample = (platformScale * (sample / randomGen.max()) - (platformScale / 2)) * ((platformScale - (2 * snakeRadius)) / (platformScale));
    float yCoord{ sample };

    bool invalidPlacement{ false };
    //Check if valid placement position
    for (SnakeSegment& segment : snake.snakeBody)
    {
        std::pair<float, float> tempCoord{};
        float x1{};
        float x2{};
        float z1{};
        float z2{};
        setBoundsFromSegment(x1, x2, z1, z2, segment);
        //Now check each of the four corners
        tempCoord = { xCoord + snakeRadius, yCoord + snakeRadius };
        if (inBox(x1, x2, z1, z2, tempCoord))
        {
            invalidPlacement = true;
            break;
        }
        tempCoord = { xCoord + snakeRadius, yCoord - snakeRadius };
        if (inBox(x1, x2, z1, z2, tempCoord))
        {
            invalidPlacement = true;
            break;
        }
        tempCoord = { xCoord - snakeRadius, yCoord + snakeRadius };
        if (inBox(x1, x2, z1, z2, tempCoord))
        {
            invalidPlacement = true;
            break;
        }
        tempCoord = { xCoord - snakeRadius, yCoord - snakeRadius };
        if (inBox(x1, x2, z1, z2, tempCoord))
        {
            invalidPlacement = true;
            break;
        }
    }
    
    if(!invalidPlacement)
        foodContainer.push_back(std::pair<float, float>{ xCoord, sample });
}

void setBoundsFromSegment(float& x1, float& x2, float& z1, float& z2, SnakeSegment& segment)
{
    switch (segment.direction)
    {
        case MOVING_UP:
            x1 = segment.frontCoord.first;
            x2 = segment.backCoord.first;
            z1 = segment.frontCoord.second - snakeRadius;
            z2 = segment.frontCoord.second + snakeRadius;
            break;
        case MOVING_DOWN:
            x1 = segment.backCoord.first;
            x2 = segment.frontCoord.first;
            z1 = segment.frontCoord.second - snakeRadius;
            z2 = segment.frontCoord.second + snakeRadius;
            break;
        case MOVING_LEFT:
            x1 = segment.frontCoord.first - snakeRadius;
            x2 = segment.frontCoord.first + snakeRadius;
            z1 = segment.backCoord.second;
            z2 = segment.frontCoord.second;
            break;
        case MOVING_RIGHT:
            x1 = segment.frontCoord.first - snakeRadius;
            x2 = segment.frontCoord.first + snakeRadius;
            z1 = segment.frontCoord.second;
            z2 = segment.backCoord.second;
            break;
    }
}