#include <iostream>
#include <array>
#include <ctime>
#include <set>

#include "CRI.hpp"
#include "random.hpp"

using namespace std;

class Pos {
	public:
	int x, y;
	Pos(int x, int y) {
		this->x = x;
		this->y = y;
	}
	bool operator< (const Pos& pos) const {	//the exact semantics of "bigger" coord don't matter since this is mostly for containers
		return ((x < pos.x) || (x == pos.x && y < pos.y));
	}
	bool operator== (const Pos& pos) const {
		return (x == pos.x) && (y == pos.y);
	}
	bool operator!= (const Pos& pos) const {
		return (x != pos.x) || (y != pos.y);
	}
};

class Car {
	public:
	int x, y;
	unsigned char glyph;
	array<uint8_t, 3> colour;
	int vectorx, vectory;
	bool wrecked = false;
	bool wrecked2 = false;
	Car(unsigned char glyph, array<uint8_t, 3> colour) {
		this->glyph = glyph;
		this->colour = colour;
		x = 0;
		y = 0;
		vectorx = 0;
		vectory = 0;
	}
};

class Racetrack {
	vector<vector<char>> currentMap;
	bool CA_getSurvive(int x, int y, int numToSurvive, int numToBorn, bool borderAlive = true, char aliveChar = '#') {
		//A cellular automata track generation
		//numToLive is either numToSurvive or numToBorn, depending on whether the tile is alive or not.
		int numToLive;
		// cout << "accessing " << x << "," << y<<  " out of " << currentMap[0].size() << "," << currentMap.size() << "\n";
		if (currentMap[y][x] == '#') {
			numToLive = numToSurvive;
		} else {
			numToLive = numToBorn;
		}
		int numLivingNeighbours = 0;
		for (int i = -1; i <= 1; ++i) {
			for (int j = -1; j <= 1; ++j) {
				//handle out of bounds depending on settings (by default the border is considered always alive)
				if (x+j < 0 || x+j >= currentMap[0].size()) {
					numLivingNeighbours += borderAlive;
					continue;
				}
				if (y+i < 0 || y+i >= currentMap.size()) {
					numLivingNeighbours += borderAlive;
					continue;
				}
				//assuming that tile is within bounds, count its status
				numLivingNeighbours += (currentMap[y+i][x+j] == aliveChar);	//true for alive converts to 1, false for dead converts to 0
			}
		}
		return numLivingNeighbours >= numToLive;
	}
	array<uint8_t, 3> getForeForChar(char c) {
		if (c == '.') {
			return {50, 50, 50};
		}
		else if (c == '#') {
			return {71, 86, 71};
		}
		else if (c == ',') {
			return {100, 100, 50};
		}
		else {
			return {255, 0, 100};
		}
	}
	array<uint8_t, 3> getBackForChar(char c) {
		if (c == '.') {
			return {0, 0, 0};
		}
		if (c == ',') {
			return {0, 0, 0};
		}
		else if (c == '#') {
			return {125, 94, 94};
		}
		else {
			return {200, 150, 0};
		}
	}

	pair<set<Pos>, bool> isLine(int x1, int y1, int x2, int y2) {	//bresenham algo
		set<Pos> line;
		int originX = x1;
		int originY = y1;
		int deltaX = abs(x2 - x1);
		int deltaY = abs(y2 - y1);

		int signX, signY;	//sign as in + or -
		if (x1 < x2) {
			signX = 1;
		} else {
			signX = -1;
		}
		if (y1 < y2) {
			signY = 1;
		} else {
			signY = -1;
		}
		int error;
		if (deltaX > deltaY) {
			error = deltaX / 2;
		} else {
			error = -deltaY / 2;
		}
		while (true) {
			/*----------*/
			line.insert(Pos(x1, y1));
			if ((x1 < 0 || x1 >= map[0].size()) || (y1 < 0 || y1 >= map.size())) {
				return make_pair(line, false);
			}
			if (map[y1][x1] != '.' && map[y1][x1] != ',') {
				return make_pair(line, false);
			}
			
			/*----------*/
			if (x1 == x2 && y1 == y2) {	//if arrived at destination
				return make_pair(line, true);
			}
			int errorCopy = error;	//make a copy of error. This is because error is potentially modified.
			if (errorCopy > -deltaX) {
				error -= deltaY;
				x1 += signX;
			}
			if (errorCopy < deltaY) {
				error += deltaX;
				y1 += signY;
			}
		}
	}
public:
	vector<Car> cars;
	vector<vector<char>> map;
	CRI * console;
	SDL_Event * e;

	void simpleCA(int dimX, int dimY, int iterations, int initCoverage, int toSurvive, int toBorn) {
		currentMap = {};
		for (int i = 0; i < dimY; ++i) {
			currentMap.push_back({});
			for (int j = 0; j < dimX; ++j) {
				//initCoverage chance of it being wall, otherwise is floor
				if (rng::randBool(((float)initCoverage)/100)) {
					currentMap[i].push_back('#');
				} else {
					currentMap[i].push_back('.');
				}
			}
		}
		for (int it = 0; it < iterations; ++it) {
			//make a temp map
			vector<vector<char>> tempMap = vector<vector<char>>(currentMap.size(), vector<char>(currentMap[0].size(), '.'));
			//set it to be the next iteration
			for (int i = 0; i < currentMap.size(); ++i) {
				for (int j = 0; j < currentMap[0].size(); ++j) {
					// cout << i << "," << j << "\n";
					if (CA_getSurvive(j, i, toSurvive, toBorn)) {
						tempMap[i][j] = '#';
					} else {
						tempMap[i][j] = '.';
					}
				}
			}
			//write to map
			currentMap = tempMap;
		}
		map = currentMap;
	}
	void digLine(int xone, int yone, int xtwo, int ytwo, int radius) {	//bresenham algo
		int i = 0;
		// for (int i = -radius; i <= radius; ++i) {
			int x2 = xtwo + i, y2 = ytwo + i;
			int x1 = xone + i, y1 = yone + i;
			int originX = x1;
			int originY = y1;
			int deltaX = abs(x2 - x1);
			int deltaY = abs(y2 - y1);

			int signX, signY;	//sign as in + or -
			if (x1 < x2) {
				signX = 1;
			} else {
				signX = -1;
			}
			if (y1 < y2) {
				signY = 1;
			} else {
				signY = -1;
			}
			int error;
			if (deltaX > deltaY) {
				error = deltaX / 2;
			} else {
				error = -deltaY / 2;
			}
			while (true) {
				/*----------*/
				if ((x1 < 0 || x1 >= currentMap[0].size()) || (y1 < 0 || y1 >= currentMap.size())) {
					break;
				}
				for (int i = -radius; i <= radius; ++i) {
					for (int j = -radius; j <= radius; ++j) {
						if ((x1 + j< 0 || x1 + j >= currentMap[0].size()) || (y1 + i < 0 || y1 +i >= currentMap.size())) {
							continue;
						}
						currentMap[y1 + i][x1 + j] = '.';
					}
				}
				/*----------*/
				if (x1 == x2 && y1 == y2) {	//if arrived at destination
					break;
				}
				int errorCopy = error;	//make a copy of error. This is because error is potentially modified.
				if (errorCopy > -deltaX) {
					error -= deltaY;
					x1 += signX;
				}
				if (errorCopy < deltaY) {
					error += deltaX;
					y1 += signY;
				}
			}
		// }
	}
	void generateRacetrack(int dimX, int dimY, int subdivisions, int roughness, int iterations = 0, int toSurvive = 0, int toBorn = 0) {
		currentMap = {};
		//generate a big stone map
		for (int i = 0; i < dimY; ++i) {
			currentMap.push_back({});
			for (int j = 0; j < dimX; ++j) {
				currentMap[i].push_back('#');
			}
		}
		vector<Pos> nodes;	//these will have tracks drawn to them
		nodes.push_back(Pos(10, 10));
		nodes.push_back(Pos(dimX - 10, 10));
		nodes.push_back(Pos(dimX - 10, dimY - 10));
		nodes.push_back(Pos(10, dimY - 10));
		//subdivide
		for (int i = 0; i < subdivisions; ++i) {
			//pick two consecutive nodes
			int node = rng::randInt(0, nodes.size());
			int nextNode = node+1;
			if (nextNode >= nodes.size()) {
				nextNode = 0;
			}
			int newX = (nodes[node].x + nodes[nextNode].x) / 2 + rng::randInt(roughness * -1, roughness);
			int newY = (nodes[node].y + nodes[nextNode].y) / 2 + rng::randInt(roughness * -1, roughness);
			//1/5 chance of being pulled towards middle
			if (rng::randBool(.2)) {
				newX = (newX + currentMap[0].size() / 2) / 2;
				newY = (newY + currentMap.size() / 2) / 2;
			}
			if ((newX > 0 && newX < currentMap[0].size()) && (newY > 0 && newY < currentMap.size())) {
				nodes.insert(nodes.begin() + nextNode, Pos(newX, newY));
			} else {
				// i--;
			}
		}
		//connect the nodes
		for (int i = 0; i < nodes.size(); ++i) {
			int nextNode = i+1;
			if (nextNode >= nodes.size()) {
				nextNode = 0;
			}
			digLine(nodes[i].x, nodes[i].y, nodes[nextNode].x, nodes[nextNode].y, 3);
		}
		//run some iterations of CA
		for (int it = 0; it < iterations; ++it) {
			//make a temp map
			vector<vector<char>> tempMap = vector<vector<char>>(currentMap.size(), vector<char>(currentMap[0].size(), '.'));
			//set it to be the next iteration
			for (int i = 0; i < currentMap.size(); ++i) {
				for (int j = 0; j < currentMap[0].size(); ++j) {
					if (CA_getSurvive(j, i, toSurvive, toBorn)) {
						tempMap[i][j] = '#';
					} else {
						tempMap[i][j] = '.';
					}
				}
			}
			//write to map
			currentMap = tempMap;
		}
		map = currentMap;
	}

	Racetrack(SDL_Event * e, CRI * console, int dimx, int dimy) {
		this->console = console;
		this->e = e;
		// simpleCA(dimx, dimy, 5, 40, 4, 5);
		generateRacetrack(dimx, dimy, 20, 4, 3, 5, 4);
	}

	void printMap(int pointerx = -1, int pointery = -1, Car * c = nullptr, set<Pos> line = {}) {
		for (int i = 0; i < map.size(); ++i) {
			for (int j = 0; j < map[0].size(); ++j) {
				array<uint8_t, 3> back = getBackForChar(map[i][j]);
				if (line.count(Pos(j, i)) == 1) {
					if (map[i][j] == '.' || map[i][j] == ',') {
						back = {255, 255, 0};
					} else {
						back = {255, 0, 0};
					}
				} else if (c != nullptr) {
					int x = c->x + c->vectorx;
					int y = c->y + c->vectory;
					if (abs(i - y) <= 1 && abs(j - x) <= 1) {
						if (map[i][j] == '.' || map[i][j] == ',') {
							// back = {0, 100, 0};
							back = console->approachColour(c->colour, {0,0,0}, .5);
						} else {
							// back = {100, 0, 0};
							back = console->approachColour(c->colour, {100,0,0}, .8);
						}
					}
				}
				console->putC(j, i, map[i][j], getForeForChar(map[i][j]), back);
			}
		}
		for (int i = 0; i < cars.size(); ++i) {
			if (cars[i].wrecked) {
				unsigned char fireGlyph = rng::randElement<unsigned char>({94, 94, 30});
				array<uint8_t, 3> fireColour = {rng::randInt(200, 255),rng::randInt(50, 255),rng::randInt(0, 200)};
				console->putC(cars[i].x, cars[i].y, fireGlyph, fireColour, getBackForChar(map[cars[i].y][cars[i].x]));
			} else {
				console->putC(cars[i].x, cars[i].y, cars[i].glyph, cars[i].colour, getBackForChar(map[cars[i].y][cars[i].x]));
			}
		}
		array<uint8_t, 3> back;
		if (pointerx != -1) {
			if (map[pointery][pointerx] == '.' || map[pointery][pointerx] == ',') {
				back = {100, 100, 0};
			} else {
				back = {255, 0, 0};
			}
			console->putC(pointerx, pointery, 'X', {255, 255, 0}, back);
		}
		console->render();
	}

   /* ----------CARS + SETUP---------- */
	void addCar(char glyph, array<uint8_t, 3> colour, int x = -1, int y = -1) {
		cars.emplace_back(glyph, colour);
		if (x != -1) {
			cars[cars.size() - 1].x = x;
			cars[cars.size() - 1].y = y;
		}
	}
   /* ----------CONTROL---------- */
	array<int, 3> selectPosWithPointer(SDL_Event * e, int startX, int startY, int maxradius = -1, Car * car = nullptr) {
		int pointerx = startX, pointery = startY;
		while (true) {
			while ((SDL_PollEvent(e)) != 0) {
				if (e->type == SDL_QUIT) {
					return {-99999, 0};
				}
				if (e->type == SDL_KEYDOWN) {
					// cout << "pressed " << SDL_GetKeyName(e->key.keysym.sym) << "\n";
					if (e->key.keysym.sym == SDLK_RETURN) {
						return {pointerx - startX, pointery - startY, isLine(car->x, car->y, pointerx, pointery).second};
					}
					if (e->key.keysym.sym == SDLK_UP) {
						if (maxradius != -1 && startY - (pointery - 1) <= maxradius) {
							pointery--;
						}
					}
					if (e->key.keysym.sym == SDLK_DOWN) {
						if (maxradius != -1 && (pointery + 1) - startY <= maxradius) {
							pointery++;
						}
					}
					if (e->key.keysym.sym == SDLK_LEFT) {
						if (maxradius != -1 && startX - (pointerx - 1) <= maxradius) {
							pointerx--;
						}
					}
					if (e->key.keysym.sym == SDLK_RIGHT) {
						if (maxradius != -1 && (pointerx + 1) - startX <= maxradius) {
							pointerx++;
						}
					}
				}
			}
			set<Pos> line = {};
			if (car != nullptr) {
				line = isLine(car->x, car->y, pointerx, pointery).first;
			}
			printMap(pointerx, pointery, car, line);
		}
	}
	void moveCar(Car * c, int x1, int y1, int x2, int y2) {	//bresenham algo
		int originX = x1;
		int originY = y1;
		int deltaX = abs(x2 - x1);
		int deltaY = abs(y2 - y1);

		int signX, signY;	//sign as in + or -
		if (x1 < x2) {
			signX = 1;
		} else {
			signX = -1;
		}
		if (y1 < y2) {
			signY = 1;
		} else {
			signY = -1;
		}
		int error;
		if (deltaX > deltaY) {
			error = deltaX / 2;
		} else {
			error = -deltaY / 2;
		}
		while (true) {
			/*----------*/
			
			if ((x1 < 0 || x1 >= map[0].size()) || (y1 < 0 || y1 >= map.size())) {
				c->wrecked = true;
				return;
			}
			if (map[y1][x1] != '.' && map[y1][x1] != ',') {
				c->wrecked = true;
				return;
			}
			c->x = x1; c->y = y1;
			map[y1][x1] = ',';
			
			/*----------*/
			if (x1 == x2 && y1 == y2) {	//if arrived at destination
				return;
			}
			int errorCopy = error;	//make a copy of error. This is because error is potentially modified.
			if (errorCopy > -deltaX) {
				error -= deltaY;
				x1 += signX;
			}
			if (errorCopy < deltaY) {
				error += deltaX;
				y1 += signY;
			}
		}
	}
	int controlCar(Car * car) {
		array<int, 3> vectorModifier = selectPosWithPointer(e, car->x + car->vectorx, car->y + car->vectory, 1, car);
		if (vectorModifier[0] == -99999) {	//if quit
			return 0;
		}
		car->vectorx += vectorModifier[0];
		car->vectory += vectorModifier[1];
		moveCar(car, car->x, car->y, car->x + car->vectorx, car->y + car->vectory);
		printMap();
		return 1;
	}
	int loopCars() {
		int numWrecked = 0;
		for (int i = 0; i < cars.size(); ++i) {
			if ((not cars[i].wrecked) && controlCar(&cars[i]) == 0) {
				return 1;
			}
			if (cars[i].wrecked) {
				numWrecked++;
				if (not cars[i].wrecked2) {
					cars[i].colour = console->approachColour(cars[i].colour, {70, 70, 50}, .8);
					cars[i].wrecked2 = true;
				}
			}
		}
		if (numWrecked == cars.size()) {
			cout << "rip ya'll";
			return 0;
		}
		return 2;
	}
};

class GameHandler {
	public:
	SDL_Event e;
	uint8_t numCars = 1;
	CRI c;
	Racetrack t = Racetrack(&e, &c, 80, 50);
	GameHandler() {
		c.init(80,50);
		c.setConsoleTitle("Racing game");
	}
	void printTitleScreen() {
		c.clear();
		int xTitle = 33;
		char arrowright = 27, arrowleft = 26;
		c.putString(xTitle - 5, 10, "-                     -", {0, 255, 255});
		c.putString(xTitle - 3, 10, "R A C I N G ", {255, 0, 0});
		c.putString(xTitle + 9, 10, "G A M E", {255, 255, 0});
		c.putString(xTitle - 18, 12, "Use the arrow keys to change your velocity vector", {100, 100, 100});
		c.putString(xTitle - 2, 15, "Number of cars:", {100, 100, 100});
		c.putString(xTitle + 14, 15, to_string((int)numCars), {255, 255, 100});
		string change = "[Change with ";change+= arrowright;change+=" ";change+=arrowleft;change+="]";
		c.putString(xTitle - 2, 16, change, {100, 100, 0});
		c.putString(xTitle - 1, 19, "[ENTER]: Begin!", {255, 255, 100});
		c.render();
	}
	int getInput(SDL_Event * e) {
		while (true) {
			printTitleScreen();
			while ((SDL_PollEvent(e)) != 0) {
				if (e->type == SDL_QUIT) {
					return -1;
				}
				if (e->type == SDL_KEYDOWN) {
					if (e->key.keysym.sym == SDLK_RETURN) {
						return 1;
					}
					if (e->key.keysym.sym == SDLK_LEFT) {
						numCars--;
					}
					if (e->key.keysym.sym == SDLK_RIGHT) {
						numCars++;
					}
				}
			}
		}
	}
	void quit() {
		c.quit();
	}
	void run() {
		while (true) {
			if (getInput(&e) == -1) {
				quit();
				return;
			} else {
				t = Racetrack(&e, &c, 80, 50);
				for (int i = 0; i < numCars; ++i) {
					char glyph = (rng::randInt(1, 175));
					if (glyph == ' ') {glyph = '@';};
					array<uint8_t, 3> colour = {rng::randInt(100, 255),rng::randInt(100, 255),rng::randInt(100, 255)};
					t.addCar(glyph, colour, 12 + rng::randInt(-2, 2), 12 + rng::randInt(-2, 2));
				}
				while (true) {
					int isquit = t.loopCars();
					if (isquit == 1) {
						quit();
						return;
					} else if (isquit == 0) {
						break;
					}
				}
			}
		}
	}
};

int main(int argc, char* args[]) {
	cout << "[_!_]beginning execution\n";
	srand(time(0));
	GameHandler g = GameHandler();
	g.run();
	return 0;
}