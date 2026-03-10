# Bangalore Metro Route Optimizer — Makefile
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -pthread -Wall -Wextra -Wno-unused-parameter
INCLUDES = -I backend/
SRC      = backend/main.cpp
OUT      = build/metro_server

.PHONY: all build run clean help test

all: build

build:
	@mkdir -p build
	@echo "→ Compiling C++ backend..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(OUT)
	@echo "✓ Built: $(OUT)"

run: build
	@echo "→ Starting server at http://localhost:8080"
	@cd build && ./metro_server

clean:
	rm -rf build/
	@echo "✓ Cleaned"

test: build
	@cd build && ./metro_server &
	@sleep 1
	@echo "\n=== GET /api/stations ==="
	@curl -s http://localhost:8080/api/stations | python3 -c \
	  "import json,sys; d=json.load(sys.stdin); print(f'{len(d)} stations, transfers: {[s[\"name\"] for s in d if s[\"isTransfer\"]]}')"
	@echo "\n=== POST /api/route (no traffic) ==="
	@curl -s -X POST http://localhost:8080/api/route \
	  -H "Content-Type: application/json" \
	  -d '{"start":"ws","end":"con","traffic":1.0,"k":3}' | python3 -c \
	  "import json,sys; d=json.load(sys.stdin); r=d['routes'][0]; print(f'{d[\"routeCount\"]} routes | Best: {r[\"totalDist\"]}km, {r[\"totalTime\"]}min, {r[\"numStations\"]} stations')"
	@echo "\n=== POST /api/route (peak traffic ×2.5) ==="
	@curl -s -X POST http://localhost:8080/api/route \
	  -H "Content-Type: application/json" \
	  -d '{"start":"ind","end":"ban","traffic":2.5,"k":2}' | python3 -c \
	  "import json,sys; d=json.load(sys.stdin); r=d['routes'][0]; print(f'{d[\"routeCount\"]} routes | Best: {r[\"totalDist\"]}km, {r[\"totalTime\"]}min')"
	@pkill metro_server 2>/dev/null || true

help:
	@echo ""
	@echo "  Bangalore Metro Route Optimizer"
	@echo "  ================================"
	@echo "  make build   — Compile C++ backend"
	@echo "  make run     — Build + start server (http://localhost:8080)"
	@echo "  make test    — Build + run API tests"
	@echo "  make clean   — Remove build artifacts"
	@echo ""
	@echo "  API Endpoints:"
	@echo "    GET  http://localhost:8080/api/stations"
	@echo "    POST http://localhost:8080/api/route"
	@echo "      Body: { \"start\":\"ws\", \"end\":\"con\", \"traffic\":1.5, \"k\":3 }"
	@echo ""
