# OpenCog Core Integration Guide

## Overview

This guide explains how to integrate the low-level OpenCog C/C++ core with the existing TypeScript implementation.

## Build the Core

```bash
cd opencog-core
make clean
make all
make test
```

## Integration Options

### Option 1: Node.js Native Addon (Recommended)

Create a Node.js native addon using N-API to bridge C and JavaScript:

**1. Install dependencies:**
```bash
npm install node-gyp node-addon-api
```

**2. Create `binding.gyp`:**
```json
{
  "targets": [{
    "target_name": "opencog_native",
    "sources": [
      "native/opencog_addon.cpp"
    ],
    "include_dirs": [
      "<!@(node -p \"require('node-addon-api').include\")",
      "opencog-core/include"
    ],
    "libraries": [
      "../opencog-core/lib/libopencog_core.a",
      "-lpthread",
      "-lrt"
    ],
    "cflags!": [ "-fno-exceptions" ],
    "cflags_cc!": [ "-fno-exceptions" ],
    "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
  }]
}
```

**3. Create `native/opencog_addon.cpp`:**
```cpp
#include <napi.h>
extern "C" {
    #include "atom.h"
    #include "distributed.h"
}

class AtomSpaceWrapper : public Napi::ObjectWrap<AtomSpaceWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    AtomSpaceWrapper(const Napi::CallbackInfo& info);
    ~AtomSpaceWrapper();

private:
    atomspace_t* space_;
    
    Napi::Value CreateAtom(const Napi::CallbackInfo& info);
    Napi::Value GetAtom(const Napi::CallbackInfo& info);
};

AtomSpaceWrapper::AtomSpaceWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<AtomSpaceWrapper>(info) {
    Napi::Env env = info.Env();
    
    uint32_t node_id = info[0].As<Napi::Number>().Uint32Value();
    space_ = atomspace_create(node_id);
}

AtomSpaceWrapper::~AtomSpaceWrapper() {
    if (space_) {
        atomspace_destroy(space_);
    }
}

Napi::Value AtomSpaceWrapper::CreateAtom(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    std::string type_str = info[0].As<Napi::String>().Utf8Value();
    std::string name = info[1].As<Napi::String>().Utf8Value();
    
    atom_type_t type = ATOM_TYPE_CONCEPT; // Map from string
    atom_handle_t* handle = atom_create(space_, type, name.c_str());
    
    return Napi::Number::New(env, handle->id);
}

Napi::Object AtomSpaceWrapper::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "AtomSpace", {
        InstanceMethod("createAtom", &AtomSpaceWrapper::CreateAtom)
    });
    
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    
    exports.Set("AtomSpace", func);
    return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return AtomSpaceWrapper::Init(env, exports);
}

NODE_API_MODULE(opencog_native, Init)
```

**4. Use from TypeScript:**
```typescript
import { AtomSpace } from './build/Release/opencog_native';

const space = new AtomSpace(1); // node_id = 1
const atomId = space.createAtom('concept', 'TestConcept');
console.log('Created atom with ID:', atomId);
```

### Option 2: Unix Domain Sockets

For process isolation, use Unix domain sockets:

**1. Start C server:**
```c
// In opencog-core/src/ipc_server.c
void start_ipc_server(atomspace_t* space) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/opencog.sock"
    };
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);
    
    while (1) {
        int client = accept(sock, NULL, NULL);
        handle_client(client, space);
    }
}
```

**2. Connect from TypeScript:**
```typescript
import * as net from 'net';

class OpenCogClient {
    private socket: net.Socket;
    
    connect(): Promise<void> {
        return new Promise((resolve) => {
            this.socket = net.createConnection('/tmp/opencog.sock');
            this.socket.on('connect', resolve);
        });
    }
    
    async createAtom(type: string, name: string): Promise<number> {
        const cmd = Buffer.from(JSON.stringify({ cmd: 'create', type, name }));
        this.socket.write(cmd);
        
        return new Promise((resolve) => {
            this.socket.once('data', (data) => {
                const result = JSON.parse(data.toString());
                resolve(result.atomId);
            });
        });
    }
}
```

### Option 3: Shared Memory

For highest performance:

**1. Create shared memory segment in C:**
```c
shared_memory_t* create_command_shm() {
    return shared_memory_create(1024 * 1024); // 1MB
}
```

**2. Access from TypeScript via FFI:**
```bash
npm install ffi-napi ref-napi
```

```typescript
import * as ffi from 'ffi-napi';
import * as ref from 'ref-napi';

const libopencog = ffi.Library('opencog-core/lib/libopencog_core.so', {
    'atomspace_create': ['pointer', ['uint32']],
    'atom_create': ['pointer', ['pointer', 'int', 'string']],
    'atom_get_tv': ['double', ['pointer']]
});

const space = libopencog.atomspace_create(1);
const atom = libopencog.atom_create(space, 0, 'TestConcept');
```

## Performance Comparison

| Method | Latency | Throughput | Isolation | Complexity |
|--------|---------|------------|-----------|------------|
| N-API | ~1-5μs | Very High | Same Process | Medium |
| Unix Socket | ~50-100μs | High | Separate Process | Low |
| Shared Memory | ~100ns | Very High | Shared Memory | High |

## Recommended Architecture

```
┌─────────────────────────────────────┐
│   Theia IDE Frontend (Browser)     │
└──────────────┬──────────────────────┘
               │ WebSocket
┌──────────────▼──────────────────────┐
│  TypeScript Backend (Node.js)       │
│  - Theia Extension                  │
│  - AI Agents                        │
└──────────────┬──────────────────────┘
               │ N-API
┌──────────────▼──────────────────────┐
│  OpenCog Core (C/C++)               │
│  - AtomSpace                        │
│  - Distributed Coordination         │
│  - Cognitive Grammar Parser         │
└──────────────┬──────────────────────┘
               │ IPC/Sockets
┌──────────────▼──────────────────────┐
│  Distributed Nodes                  │
│  - Remote AtomSpaces                │
│  - Consensus Protocol               │
└─────────────────────────────────────┘
```

## Next Steps

1. Build the native addon: `node-gyp configure && node-gyp build`
2. Create TypeScript wrapper classes
3. Integrate with existing `AtomSpaceService`
4. Add performance monitoring
5. Implement distributed node discovery
6. Add cognitive grammar integration

## Example Full Integration

```typescript
// opencog-bridge.ts
import { AtomSpace } from './build/Release/opencog_native';

export class OpenCogBridge {
    private nativeSpace: any;
    
    constructor(nodeId: number) {
        this.nativeSpace = new AtomSpace(nodeId);
    }
    
    createConcept(name: string, tv?: TruthValue): number {
        const atomId = this.nativeSpace.createAtom('concept', name);
        if (tv) {
            this.nativeSpace.setTruthValue(atomId, tv.strength, tv.confidence);
        }
        return atomId;
    }
    
    query(pattern: Pattern): QueryResult[] {
        // Call native pattern matcher
        return this.nativeSpace.matchPattern(pattern);
    }
}

// Use in existing AtomSpaceService
@injectable()
export class EnhancedAtomSpaceService extends AtomSpaceService {
    private bridge: OpenCogBridge;
    
    constructor() {
        super();
        this.bridge = new OpenCogBridge(1);
    }
    
    async createAtom(type: string, name: string): Promise<number> {
        // Use high-performance C implementation
        return this.bridge.createConcept(name);
    }
}
```

## Troubleshooting

**Build errors:**
- Ensure `make all` succeeds in opencog-core/
- Check that node-gyp can find headers
- Verify library paths in binding.gyp

**Runtime errors:**
- Check LD_LIBRARY_PATH includes opencog-core/lib/
- Verify atomspace initialization
- Check for memory leaks with valgrind

**Performance issues:**
- Profile with perf/gprof
- Enable assembly optimizations (-march=native)
- Use shared memory for bulk operations
