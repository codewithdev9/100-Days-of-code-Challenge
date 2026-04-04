# CyberpunkTodoList — Project Blueprint
> Version: 1.0.0 | Architecture: Microservices-Inspired Monorepo

---

## Directory Tree

```
CyberpunkTodoList/
├── STRUCTURE.md                    ← This file. Master reference for all components.
├── docker-compose.yml              ← Orchestrates backend, DB, Redis, frontend dev server
├── .env.example                    ← Environment variable template
│
├── frontend/                       ← Vanilla JS / HTML5 SPA (no build step required)
│   ├── index.html                  ← App shell, PWA manifest link, font imports
│   ├── css/
│   │   ├── variables.css           ← CSS custom properties: neon palette, glows, fonts
│   │   ├── base.css                ← Reset, scrollbar styling, selection highlights
│   │   ├── layout.css              ← Grid/flex layouts, responsive breakpoints
│   │   ├── components.css          ← Cards, modals, buttons, badges, tags
│   │   ├── animations.css          ← Glitch, flicker, scanline, neon-pulse keyframes
│   │   └── cyberpunk-theme.css     ← Master import + dark glassmorphism surfaces
│   ├── js/
│   │   ├── app.js                  ← Entry point; router init, event bus, app state
│   │   ├── api.js                  ← Fetch wrapper; JWT injection, error handling
│   │   ├── auth.js                 ← Features 1–3: Login, register, session management
│   │   ├── tasks.js                ← Features 4–15: Full task CRUD + subtasks + stars
│   │   ├── organize.js             ← Features 16–22: Categories, search, sort, tags
│   │   ├── ui.js                   ← Features 23–35: Drag-drop, modals, progress, sound
│   │   ├── utility.js              ← Features 36–50: Export, undo/redo, timestamps
│   │   ├── ai.js                   ← Features 51–58: NLP parsing, smart suggestions
│   │   ├── collaboration.js        ← Features 59–68: WS client, chat, leaderboards
│   │   ├── productivity.js         ← Features 69–78: Pomodoro, Gantt, Kanban, heatmap
│   │   ├── integrations.js         ← Features 79–90: OAuth, calendar, webhooks
│   │   ├── advanced.js             ← Features 91–100: PWA, biometric, encryption, API
│   │   └── state.js                ← Centralized state store (localStorage + memory)
│   ├── components/
│   │   ├── TaskCard.js             ← Reusable task card component
│   │   ├── Modal.js                ← Hacker-style confirmation/form modals
│   │   ├── KanbanBoard.js          ← Kanban view component
│   │   ├── GanttChart.js           ← Gantt chart renderer (Canvas API)
│   │   ├── Heatmap.js              ← GitHub-style activity heatmap
│   │   ├── PomodoroTimer.js        ← Pomodoro widget
│   │   └── ChatPanel.js            ← Real-time in-app chat UI
│   └── assets/
│       └── sounds/                 ← Sci-fi UI click/complete/error WAV files
│
├── backend/                        ← Python FastAPI application
│   ├── main.py                     ← FastAPI app init, CORS, router mounts, WS setup
│   ├── config.py                   ← Pydantic settings; reads .env
│   ├── requirements.txt            ← All Python dependencies
│   ├── routes/
│   │   ├── auth.py                 ← POST /auth/register, /auth/login, /auth/refresh
│   │   ├── tasks.py                ← Full CRUD: GET/POST/PUT/DELETE /tasks
│   │   ├── categories.py           ← GET/POST/DELETE /categories
│   │   ├── tags.py                 ← Tag management endpoints
│   │   ├── users.py                ← GET/PUT /users/me, /users/leaderboard
│   │   ├── ai.py                   ← POST /ai/parse, /ai/suggest, /ai/prioritize
│   │   ├── collaboration.py        ← POST /collab/share, /collab/assign, /collab/chat
│   │   ├── integrations.py         ← OAuth callbacks, calendar, Slack/Discord webhooks
│   │   └── websocket.py            ← WS /ws/{room_id} endpoint for real-time
│   ├── models/
│   │   ├── user.py                 ← SQLAlchemy User model + Pydantic schemas
│   │   ├── task.py                 ← Task, SubTask models with all fields
│   │   ├── category.py             ← Category model
│   │   ├── tag.py                  ← Tag model (many-to-many with tasks)
│   │   ├── message.py              ← Chat message model
│   │   └── activity.py             ← Activity log for heatmap/XP
│   ├── middleware/
│   │   ├── auth_middleware.py      ← JWT verification, role guards
│   │   ├── rate_limiter.py         ← Redis-backed rate limiting
│   │   └── logger.py               ← Structured request/response logging
│   ├── services/
│   │   ├── jwt_service.py          ← Token create/decode/refresh logic
│   │   ├── cache_service.py        ← Redis get/set/invalidate wrappers
│   │   ├── ai_service.py           ← OpenAI / local NLP integration
│   │   ├── email_service.py        ← Email-to-task, notification emails
│   │   ├── encryption_service.py   ← AES-256 field-level task encryption
│   │   └── gamification_service.py ← XP calculation, badge awards
│   └── utils/
│       ├── database.py             ← SQLAlchemy engine + session factory
│       ├── redis_client.py         ← Redis connection pool
│       └── validators.py           ← Reusable Pydantic validators
│
├── database/
│   ├── migrations/
│   │   ├── 001_create_users.sql
│   │   ├── 002_create_tasks.sql
│   │   ├── 003_create_categories_tags.sql
│   │   ├── 004_create_collaboration.sql
│   │   └── 005_create_activity.sql
│   └── seeds/
│       └── demo_data.sql           ← Sample tasks/users for dev environment
│
└── assets/
    ├── icons/                      ← SVG icon set (neon-styled)
    └── fonts/                      ← Self-hosted font files (fallback)
```

---

## Communication Architecture

```
[Browser / PWA]
      │  HTTPS REST (JSON)     WebSocket (wss://)
      ▼                              ▼
[FastAPI Backend]  ←──────────────────────────
      │                     │
      ▼                     ▼
[PostgreSQL DB]         [Redis Cache]
  (persistent)        (sessions, rate-limit,
                        pub/sub for WS)
```

### Auth Flow
1. Client POSTs credentials → `/auth/login`
2. Backend validates, returns `access_token` (JWT, 15min) + `refresh_token` (7d)
3. Every API request: `Authorization: Bearer <access_token>`
4. `api.js` auto-refreshes token on 401 response

### Real-time Flow
1. Client opens `WebSocket /ws/{room_id}` with JWT in query param
2. `websocket.py` validates, joins Redis pub/sub channel
3. Task mutations publish events → all room subscribers receive updates

---

## Feature Map (100 Features)

| # | Feature | Frontend File | Backend Route |
|---|---------|--------------|---------------|
| 1 | User Registration | auth.js | routes/auth.py |
| 2 | JWT Login | auth.js | routes/auth.py |
| 3 | Session Management | auth.js | middleware/auth_middleware.py |
| 4 | Add Task | tasks.js | routes/tasks.py POST |
| 5 | Edit Task | tasks.js | routes/tasks.py PUT |
| 6 | Delete Task | tasks.js | routes/tasks.py DELETE |
| 7 | Bulk Delete | tasks.js | routes/tasks.py DELETE bulk |
| 8 | Copy Task | tasks.js | routes/tasks.py POST /copy |
| 9 | Sub-tasks | tasks.js | routes/tasks.py subtask endpoints |
| 10 | Status Toggle | tasks.js | routes/tasks.py PATCH status |
| 11 | Star/Favorite | tasks.js | routes/tasks.py PATCH star |
| 12 | Password-protect task | tasks.js | services/encryption_service.py |
| 13 | Categories | organize.js | routes/categories.py |
| 14 | Search Bar | organize.js | routes/tasks.py GET ?q= |
| 15 | Multi-level Sort | organize.js | routes/tasks.py GET ?sort= |
| 16 | Priority Labels | organize.js | models/task.py |
| 17 | Tags (#Urgent) | organize.js | routes/tags.py |
| 18 | Cyberpunk Dark Mode | css/variables.css | — |
| 19 | Responsive Design | css/layout.css | — |
| 20 | Drag-and-Drop | ui.js | routes/tasks.py PATCH order |
| 21 | Progress Bars | ui.js | — |
| 22 | Confirmation Modals | ui.js | — |
| 23 | Date Pickers | ui.js | — |
| 24 | Sound Effects | ui.js | — |
| 25 | Export to PDF | utility.js | — |
| 26 | Print View | utility.js | — |
| 27 | LocalStorage Sync | state.js | — |
| 28 | Undo/Redo | utility.js | — |
| 29 | Timestamps | tasks.js | models/task.py |
| 30 | Motivational Quotes | ui.js | — |
| 31 | Glitch on Complete | animations.css | — |
| 32 | Neon Glow CSS vars | css/variables.css | — |
| 33 | Glassmorphism UI | css/components.css | — |
| 34 | Scanline Effect | css/animations.css | — |
| 35 | Terminal Font | css/variables.css | — |
| 36 | Task Timestamps | tasks.js | models/task.py |
| 37 | Activity Log | utility.js | routes/users.py |
| 38 | Keyboard Shortcuts | ui.js | — |
| 39 | Quick-add Bar | ui.js | routes/tasks.py |
| 40 | Filter Panel | organize.js | — |
| 41 | Bulk Status Update | tasks.js | routes/tasks.py |
| 42 | Color Themes | css/variables.css | — |
| 43 | Compact/Full View | ui.js | — |
| 44 | Empty State Quotes | ui.js | — |
| 45 | Neon Priority Colors | css/components.css | — |
| 46 | Task Notes (Markdown)| tasks.js | models/task.py |
| 47 | Due Date Warnings | ui.js | — |
| 48 | Recurring Tasks | tasks.js | routes/tasks.py |
| 49 | Archive Tasks | tasks.js | routes/tasks.py |
| 50 | Stats Dashboard | utility.js | routes/users.py |
| 51 | NLP Task Parsing | ai.js | routes/ai.py |
| 52 | Smart Suggestions | ai.js | routes/ai.py |
| 53 | Auto-Prioritization | ai.js | routes/ai.py |
| 54 | OmniTask Engine | ai.js | services/ai_service.py |
| 55 | Voice Input (STT) | ai.js | — |
| 56 | AI Task Breakdown | ai.js | routes/ai.py |
| 57 | Context-aware Tags | ai.js | routes/ai.py |
| 58 | Deadline Prediction | ai.js | services/ai_service.py |
| 59 | Team Sharing | collaboration.js | routes/collaboration.py |
| 60 | Task Assignment | collaboration.js | routes/collaboration.py |
| 61 | In-app Chat | components/ChatPanel.js | routes/websocket.py |
| 62 | Real-time Updates | collaboration.js | routes/websocket.py |
| 63 | Leaderboards | collaboration.js | routes/users.py |
| 64 | XP / Badges | collaboration.js | services/gamification_service.py |
| 65 | Avatar System | collaboration.js | routes/users.py |
| 66 | Task Comments | collaboration.js | routes/tasks.py |
| 67 | @Mentions | collaboration.js | routes/collaboration.py |
| 68 | Notification Center | ui.js | routes/users.py |
| 69 | Pomodoro Timer | components/PomodoroTimer.js | — |
| 70 | Time Tracking | productivity.js | routes/tasks.py |
| 71 | Gantt Chart | components/GanttChart.js | routes/tasks.py |
| 72 | Kanban Board | components/KanbanBoard.js | routes/tasks.py |
| 73 | Activity Heatmap | components/Heatmap.js | routes/users.py |
| 74 | Time Reports | productivity.js | routes/users.py |
| 75 | Focus Mode | productivity.js | — |
| 76 | Sprint Planning | productivity.js | routes/tasks.py |
| 77 | Velocity Metrics | productivity.js | routes/users.py |
| 78 | Calendar View | productivity.js | routes/tasks.py |
| 79 | Google OAuth | integrations.js | routes/integrations.py |
| 80 | GitHub OAuth | integrations.js | routes/integrations.py |
| 81 | Google Calendar Sync | integrations.js | routes/integrations.py |
| 82 | Slack Webhook | integrations.js | routes/integrations.py |
| 83 | Discord Webhook | integrations.js | routes/integrations.py |
| 84 | Email-to-Task | — | routes/integrations.py |
| 85 | Chrome Extension API | advanced.js | routes/tasks.py |
| 86 | Public API Docs | — | main.py (OpenAPI) |
| 87 | API Key Management | advanced.js | routes/users.py |
| 88 | Offline Mode (PWA) | advanced.js | — |
| 89 | Biometric Auth Sim | advanced.js | — |
| 90 | Field Encryption | advanced.js | services/encryption_service.py |
| 91 | Markdown Rendering | tasks.js | — |
| 92 | Import Tasks (JSON/CSV)| utility.js | routes/tasks.py |
| 93 | Data Export (JSON) | utility.js | routes/tasks.py |
| 94 | Two-Factor Auth | auth.js | routes/auth.py |
| 95 | Audit Trail | utility.js | models/activity.py |
| 96 | Rate Limiting | — | middleware/rate_limiter.py |
| 97 | CORS Security | — | main.py |
| 98 | Input Sanitization | — | utils/validators.py |
| 99 | Redis Caching | — | services/cache_service.py |
| 100| Auto-backup & Restore | utility.js | routes/tasks.py |