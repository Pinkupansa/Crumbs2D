// =====================================================================================================================
// Profiler.h — profileur d'images single-header, avec visualisation Dear ImGui.
//
// UTILISATION
//   L'implémentation est en deux parties, chacune à compiler dans UN SEUL fichier .cpp du programme :
//       #define PROFILER_IMPLEMENTATION          // enregistrement, statistiques, export : AUCUNE dépendance à ImGui
//       #include "Profiler.h"
//   et, seulement là où ImGui est disponible :
//       #define PROFILER_IMGUI_IMPLEMENTATION    // la fenêtre prof::DrawWindow
//       #include "Profiler.h"
//   Un programme sans interface (tests, benchmark) n'a besoin que de la première.
//   Partout ailleurs, un simple #include "Profiler.h" suffit. Les deux parties sont compilées même si l'en-tête
//   a déjà été inclus plus haut dans le même fichier.
//
//   PROFILE_FRAME();                    // une fois par image, HORS de tout scope profilé (début ou fin de boucle)
//   PROFILE_SCOPE("BroadPhase");        // mesure le bloc courant, jusqu'à la fin du scope
//   PROFILE_FUNCTION();                 // idem, avec le nom de la fonction
//   PROFILE_BEGIN("Chargement");        // ouvre une zone hors scope C++...
//   PROFILE_END();                      // ...et ferme la dernière zone ouverte par PROFILE_BEGIN
//   PROFILE_VALUE("Paires", n);         // enregistre une valeur par image (compteur, mémoire, nombre d'objets...)
//
//   prof::DrawWindow();                 // dessine la fenêtre ImGui (entre ImGui::NewFrame et ImGui::Render)
//   prof::GetZoneStats("BroadPhase");   // lit les statistiques d'une zone depuis le code (scènes, benchmarks)
//   prof::GetFrameStats();              // idem pour l'image entière
//   prof::ExportChromeTrace("t.json");  // exporte l'historique, à ouvrir dans https://ui.perfetto.dev
//
//   #define PROFILER_ENABLED 0 avant l'include (ou en option de compilation) : toutes les macros disparaissent.
//
// LIMITES
//   - prof::DrawWindow n'existe que si PROFILER_IMGUI_IMPLEMENTATION est compilée quelque part.
//   - Mono-thread : seul le thread qui appelle le profileur en premier est enregistré, les autres sont ignorés.
//     DrawWindow, GetZoneStats et ExportChromeTrace s'appellent depuis ce même thread.
//   - Les noms passés aux macros doivent vivre aussi longtemps que le programme (littéraux de chaîne).
//
// CHOIX DE VISUALISATION (principes de dataviz éprouvés)
//   - Série temporelle en barres verticales avec une ligne de référence (le budget) : on voit d'un coup d'œil
//     les images qui dépassent, et les pics isolés qu'une moyenne cache.
//   - Échelle verticale robuste (p99 et budget), pour qu'un pic unique n'écrase pas tout le graphe.
//   - Percentile (p95) et maximum à côté de la moyenne : la moyenne seule masque les saccades.
//   - Comparaisons en barres horizontales triées (part de l'image), jamais en camembert.
//   - Une couleur stable par zone, la même dans toutes les vues ; la couleur vive est réservée aux alertes.
//   - Étiquetage direct (noms dans les rectangles du flame graph), unités dans les en-têtes.
//   - Petites tendances (sparklines) dans le tableau, pour lire l'évolution sans changer de vue.
// =====================================================================================================================
#ifndef PROFILER_H_INCLUDED
#define PROFILER_H_INCLUDED

#include <cstdint>

#ifndef PROFILER_ENABLED
#define PROFILER_ENABLED 1
#endif

namespace prof
{
using ZoneId = uint32_t;
using CounterId = uint32_t;

struct ZoneToken
{
    uint64_t FrameSerial;
    uint32_t EventIndex;
};

// Statistiques d'une zone (ou de l'image) sur les dernières images observées, en millisecondes.
struct ZoneStats
{
    bool Found = false; // faux si aucune zone de ce nom n'a été mesurée
    float AvgMs = 0.0f;
    float P95Ms = 0.0f;
    float MaxMs = 0.0f;
    float LastMs = 0.0f;
    float AvgCalls = 0.0f; // nombre moyen d'appels par image
};

ZoneId RegisterZone(const char* name, const char* file, int line);
CounterId RegisterCounter(const char* name);
ZoneToken BeginZone(ZoneId zone);
void EndZone(ZoneToken token);
void BeginManualZone(ZoneId zone);
void EndManualZone();
void SetCounter(CounterId counter, double value);
void NewFrame();

void DrawWindow(const char* title = "Profiler", bool* open = nullptr);
void SetBudgetMs(float budgetMs);
void Reset();

// window <= 0 : la fenêtre d'observation réglée dans l'interface.
ZoneStats GetZoneStats(const char* name, int window = 0);
ZoneStats GetFrameStats(int window = 0);
bool ExportChromeTrace(const char* path);

class ScopedZone
{
public:
    explicit ScopedZone(ZoneId zone) : m_Token(BeginZone(zone)) {}
    ~ScopedZone() { EndZone(m_Token); }
    ScopedZone(const ScopedZone&) = delete;
    ScopedZone& operator=(const ScopedZone&) = delete;

private:
    ZoneToken m_Token;
};
} // namespace prof

#define PROFILER_CONCAT_INNER(a, b) a##b
#define PROFILER_CONCAT(a, b) PROFILER_CONCAT_INNER(a, b)

#if PROFILER_ENABLED
// __COUNTER__ donne un suffixe unique, même avec plusieurs macros sur une seule ligne.
#define PROFILER_SCOPE_IMPL(name, id)                                                                                  \
    static const ::prof::ZoneId PROFILER_CONCAT(profZone_, id) = ::prof::RegisterZone(name, __FILE__, __LINE__);       \
    const ::prof::ScopedZone PROFILER_CONCAT(profScope_, id)(PROFILER_CONCAT(profZone_, id))
#define PROFILE_SCOPE(name) PROFILER_SCOPE_IMPL(name, __COUNTER__)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#define PROFILE_BEGIN(name)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        static const ::prof::ZoneId profManualZone_ = ::prof::RegisterZone(name, __FILE__, __LINE__);                  \
        ::prof::BeginManualZone(profManualZone_);                                                                      \
    } while (0)
#define PROFILE_END() ::prof::EndManualZone()
#define PROFILE_FRAME() ::prof::NewFrame()
#define PROFILE_VALUE(name, value)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        static const ::prof::CounterId profCounter_ = ::prof::RegisterCounter(name);                                   \
        ::prof::SetCounter(profCounter_, static_cast<double>(value));                                                  \
    } while (0)
#else
#define PROFILE_SCOPE(name) ((void)0)
#define PROFILE_FUNCTION() ((void)0)
#define PROFILE_BEGIN(name) ((void)0)
#define PROFILE_END() ((void)0)
#define PROFILE_FRAME() ((void)0)
#define PROFILE_VALUE(name, value) ((void)0)
#endif

#endif // PROFILER_H_INCLUDED

// =====================================================================================================================
// DÉCLARATIONS INTERNES (communes aux deux implémentations, sans ImGui)
// =====================================================================================================================
#if (defined(PROFILER_IMPLEMENTATION) || defined(PROFILER_IMGUI_IMPLEMENTATION)) && !defined(PROFILER_DETAIL_INCLUDED)
#define PROFILER_DETAIL_INCLUDED

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace prof
{
namespace detail
{
constexpr int kHistory = 300;             // images conservées
constexpr ZoneId kFrameZone = UINT32_MAX; // zone fictive de la racine (l'image entière)

inline int64_t NowNs()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

inline float NsToMs(int64_t ns) { return static_cast<float>(static_cast<double>(ns) * 1e-6); }

struct ZoneInfo
{
    const char* Name;
    const char* File;
    int Line;
};

struct Event
{
    ZoneId Zone;
    int32_t Parent; // indice de l'événement parent dans l'image, -1 au premier niveau
    int32_t Depth;
    int64_t Start;
    int64_t End; // 0 tant que la zone est ouverte
};

struct FrameRecord
{
    int64_t Start = 0;
    int64_t End = 0;
    std::vector<Event> Events;
};

// Un nœud de l'arbre d'appels agrégé : une zone, sous un chemin de parents donné.
struct Node
{
    ZoneId Zone = kFrameZone;
    int Parent = -1;
    int Depth = 0;
    std::vector<int> Children;
    float Total[kHistory] = {};
    float Self[kHistory] = {};
    uint16_t Calls[kHistory] = {};
};

// Statistiques par zone, tous chemins confondus (vue à plat).
struct ZoneHistory
{
    float Total[kHistory] = {};
    float Self[kHistory] = {};
    uint16_t Calls[kHistory] = {};
};

struct Counter
{
    const char* Name = "";
    float Values[kHistory] = {};
    double Current = 0.0;
};

struct Summary
{
    float Avg = 0.0f;
    float P95 = 0.0f;
    float Max = 0.0f;
    float Last = 0.0f;
};

struct State
{
    std::vector<ZoneInfo> Zones;
    std::vector<ZoneHistory> ZoneHistories;
    std::vector<Counter> Counters;
    std::vector<Node> Nodes;

    FrameRecord Frames[kHistory];
    float FrameMs[kHistory] = {};
    int64_t FrameCount = 0;

    FrameRecord Current;
    std::vector<uint32_t> OpenStack;
    std::vector<ZoneToken> ManualStack; // zones ouvertes par PROFILE_BEGIN
    uint64_t FrameSerial = 1;
    bool FrameStarted = false;

    std::thread::id Owner;
    std::once_flag OwnerOnce;

    // Protège les registres (zones, compteurs) : une zone peut être enregistrée depuis un autre thread,
    // même si ses mesures sont ignorées.
    std::mutex RegistryMutex;

    // Interface
    bool Paused = false;
    bool FollowLatest = true;
    bool PauseOnSpike = false; // mode capture : pause automatique à la première image hors budget
    int64_t SelectedFrame = -1;
    int Window = 120;
    float BudgetMs = 1000.0f / 60.0f;
    bool TreeView = true;
    char ExportPath[256] = "profile_trace.json";
    char ExportStatus[128] = "";

    // Tampons réutilisés (aucune allocation par image en régime établi)
    std::vector<int> EventNodes;
    std::vector<float> EventSelf;
    std::vector<float> Scratch;
    std::vector<float> Series;

    State() { Nodes.emplace_back(); } // racine : l'image
};

// L'unique instance, définie par PROFILER_IMPLEMENTATION.
State& Get();
void ResetLocked(State& s);
bool ExportChromeTraceLocked(State& s, const char* path);

inline int Slot(int64_t frame) { return static_cast<int>(frame % kHistory); }

// Statistiques sur les `window` dernières images d'un historique circulaire.
template <typename T> Summary Summarize(State& s, const T* history, int window)
{
    Summary summary;
    const int n = static_cast<int>(std::min<int64_t>(window, s.FrameCount));
    if (n <= 0)
        return summary;

    s.Scratch.resize(static_cast<size_t>(n));
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const float v = static_cast<float>(history[Slot(s.FrameCount - 1 - i)]);
        s.Scratch[static_cast<size_t>(i)] = v;
        sum += static_cast<double>(v);
    }
    summary.Last = s.Scratch[0];
    summary.Avg = static_cast<float>(sum / n);
    summary.Max = *std::max_element(s.Scratch.begin(), s.Scratch.end());
    const size_t p95 = static_cast<size_t>(std::ceil(0.95 * n)) - 1;
    std::nth_element(s.Scratch.begin(), s.Scratch.begin() + static_cast<std::ptrdiff_t>(p95), s.Scratch.end());
    summary.P95 = s.Scratch[p95];
    return summary;
}

// Copie chronologique (de la plus ancienne à la plus récente), pour les petites courbes.
template <typename T> const float* Chronological(State& s, const T* history, int window, int& outCount)
{
    const int n = static_cast<int>(std::min<int64_t>(window, s.FrameCount));
    s.Series.resize(static_cast<size_t>(std::max(n, 1)));
    for (int i = 0; i < n; ++i)
        s.Series[static_cast<size_t>(i)] = static_cast<float>(history[Slot(s.FrameCount - n + i)]);
    outCount = n;
    return s.Series.data();
}

inline int EffectiveWindow(const State& s, int window) { return window > 0 ? std::min(window, kHistory) : s.Window; }

inline ZoneStats ToStats(const Summary& total, const Summary& calls)
{
    ZoneStats stats;
    stats.Found = true;
    stats.AvgMs = total.Avg;
    stats.P95Ms = total.P95;
    stats.MaxMs = total.Max;
    stats.LastMs = total.Last;
    stats.AvgCalls = calls.Avg;
    return stats;
}

} // namespace detail
} // namespace prof

#endif // PROFILER_DETAIL_INCLUDED

// =====================================================================================================================
// IMPLÉMENTATION : enregistrement, statistiques, export (sans ImGui)
// =====================================================================================================================
#if defined(PROFILER_IMPLEMENTATION) && !defined(PROFILER_IMPLEMENTATION_INCLUDED)
#define PROFILER_IMPLEMENTATION_INCLUDED

#include <iterator>

namespace prof
{
namespace detail
{
State& Get()
{
    static State state;
    return state;
}

inline bool IsOwnerThread()
{
    static thread_local int cached = -1;
    if (cached < 0)
    {
        State& s = Get();
        std::call_once(s.OwnerOnce, [&] { s.Owner = std::this_thread::get_id(); });
        cached = s.Owner == std::this_thread::get_id() ? 1 : 0;
    }
    return cached == 1;
}

inline int FindOrAddChild(State& s, int parent, ZoneId zone)
{
    for (int child : s.Nodes[static_cast<size_t>(parent)].Children)
        if (s.Nodes[static_cast<size_t>(child)].Zone == zone)
            return child;

    const int index = static_cast<int>(s.Nodes.size());
    s.Nodes.emplace_back();
    Node& node = s.Nodes.back();
    node.Zone = zone;
    node.Parent = parent;
    node.Depth = s.Nodes[static_cast<size_t>(parent)].Depth + 1;
    s.Nodes[static_cast<size_t>(parent)].Children.push_back(index);
    return index;
}

inline uint16_t Increment(uint16_t value) { return value < 65535 ? static_cast<uint16_t>(value + 1) : value; }

inline void Commit(State& s)
{
    const int slot = Slot(s.FrameCount);
    FrameRecord& record = s.Frames[slot];
    record.Start = s.Current.Start;
    record.End = s.Current.End;
    std::swap(record.Events, s.Current.Events);

    const float frameMs = NsToMs(record.End - record.Start);
    s.FrameMs[slot] = frameMs;

    for (Node& node : s.Nodes)
    {
        node.Total[slot] = 0.0f;
        node.Self[slot] = 0.0f;
        node.Calls[slot] = 0;
    }
    for (ZoneHistory& zone : s.ZoneHistories)
    {
        zone.Total[slot] = 0.0f;
        zone.Self[slot] = 0.0f;
        zone.Calls[slot] = 0;
    }

    s.Nodes[0].Total[slot] = frameMs;
    s.Nodes[0].Self[slot] = frameMs;
    s.Nodes[0].Calls[slot] = 1;

    const size_t count = record.Events.size();
    s.EventNodes.resize(count);
    s.EventSelf.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        Event& e = record.Events[i];
        if (e.End == 0)
            e.End = record.End; // zone restée ouverte : fermée à la fin de l'image

        const float duration = NsToMs(e.End - e.Start);
        const int parentNode = e.Parent < 0 ? 0 : s.EventNodes[static_cast<size_t>(e.Parent)];
        const int node = FindOrAddChild(s, parentNode, e.Zone);
        s.EventNodes[i] = node;

        Node& n = s.Nodes[static_cast<size_t>(node)];
        n.Total[slot] += duration;
        n.Self[slot] += duration;
        n.Calls[slot] = Increment(n.Calls[slot]);
        s.Nodes[static_cast<size_t>(parentNode)].Self[slot] -= duration;

        s.EventSelf[i] = duration;
        if (e.Parent >= 0)
            s.EventSelf[static_cast<size_t>(e.Parent)] -= duration;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const Event& e = record.Events[i];
        ZoneHistory& zone = s.ZoneHistories[e.Zone];
        // Une zone récursive serait comptée plusieurs fois : seule l'occurrence la plus haute entre dans le total.
        bool nested = false;
        for (int32_t p = e.Parent; p >= 0; p = record.Events[static_cast<size_t>(p)].Parent)
            if (record.Events[static_cast<size_t>(p)].Zone == e.Zone)
            {
                nested = true;
                break;
            }
        if (!nested)
            zone.Total[slot] += NsToMs(e.End - e.Start);
        zone.Self[slot] += s.EventSelf[i];
        zone.Calls[slot] = Increment(zone.Calls[slot]);
    }

    for (Counter& counter : s.Counters)
        counter.Values[slot] = static_cast<float>(counter.Current);

    ++s.FrameCount;
}

// Écrit une chaîne JSON échappée (guillemets, barres obliques inverses, caractères de contrôle).
inline void WriteJsonString(std::FILE* file, const char* text)
{
    std::fputc('"', file);
    for (const char* c = text; *c; ++c)
    {
        const unsigned char ch = static_cast<unsigned char>(*c);
        if (ch == '"' || ch == '\\')
        {
            std::fputc('\\', file);
            std::fputc(ch, file);
        }
        else if (ch < 0x20)
            std::fprintf(file, "\\u%04x", ch);
        else
            std::fputc(ch, file);
    }
    std::fputc('"', file);
}

void ResetLocked(State& s)
{
    for (Node& node : s.Nodes)
    {
        std::fill(std::begin(node.Total), std::end(node.Total), 0.0f);
        std::fill(std::begin(node.Self), std::end(node.Self), 0.0f);
        std::fill(std::begin(node.Calls), std::end(node.Calls), uint16_t{0});
    }
    for (ZoneHistory& zone : s.ZoneHistories)
        zone = ZoneHistory{};
    for (Counter& counter : s.Counters)
        std::fill(std::begin(counter.Values), std::end(counter.Values), 0.0f);
    std::fill(std::begin(s.FrameMs), std::end(s.FrameMs), 0.0f);
    s.FrameCount = 0;
    s.SelectedFrame = -1;
    s.FollowLatest = true;
}

bool ExportChromeTraceLocked(State& s, const char* path)
{
    std::FILE* file = std::fopen(path, "w");
    if (!file)
        return false;

    const int n = static_cast<int>(std::min<int64_t>(kHistory, s.FrameCount));
    const int64_t origin = n > 0 ? s.Frames[Slot(s.FrameCount - n)].Start : 0;
    auto toUs = [&](int64_t ns) { return static_cast<double>(ns - origin) * 1e-3; };

    std::fputs("{\"displayTimeUnit\":\"ms\",\"traceEvents\":[\n", file);
    bool first = true;
    auto writeEvent = [&](const char* name, int64_t start, int64_t end, const char* category) {
        std::fputs(first ? "" : ",\n", file);
        first = false;
        std::fputs("{\"name\":", file);
        WriteJsonString(file, name);
        std::fprintf(
            file,
            ",\"cat\":\"%s\",\"ph\":\"X\",\"ts\":%.3f,\"dur\":%.3f,\"pid\":0,\"tid\":0}",
            category,
            toUs(start),
            static_cast<double>(end - start) * 1e-3
        );
    };

    for (int i = 0; i < n; ++i)
    {
        const FrameRecord& frame = s.Frames[Slot(s.FrameCount - n + i)];
        writeEvent("Image", frame.Start, frame.End, "frame");
        for (const Event& e : frame.Events)
            writeEvent(s.Zones[e.Zone].Name, e.Start, e.End == 0 ? frame.End : e.End, "zone");
    }
    std::fputs("\n]}\n", file);
    return std::fclose(file) == 0;
}
} // namespace detail

// =====================================================================================================================
// API
// =====================================================================================================================
ZoneId RegisterZone(const char* name, const char* file, int line)
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    // Comparaison par contenu : __FILE__ peut avoir des adresses différentes selon l'unité de compilation.
    for (ZoneId i = 0; i < s.Zones.size(); ++i)
        if (s.Zones[i].Line == line && std::strcmp(s.Zones[i].File, file) == 0 &&
            std::strcmp(s.Zones[i].Name, name) == 0)
            return i;
    const ZoneId id = static_cast<ZoneId>(s.Zones.size());
    s.Zones.push_back({name, file, line});
    s.ZoneHistories.emplace_back();
    return id;
}

CounterId RegisterCounter(const char* name)
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    for (CounterId i = 0; i < s.Counters.size(); ++i)
        if (std::strcmp(s.Counters[i].Name, name) == 0)
            return i;
    detail::Counter counter;
    counter.Name = name;
    s.Counters.push_back(counter);
    return static_cast<CounterId>(s.Counters.size() - 1);
}

ZoneToken BeginZone(ZoneId zone)
{
    if (!detail::IsOwnerThread())
        return {0, 0};
    detail::State& s = detail::Get();
    const int64_t now = detail::NowNs();
    if (!s.FrameStarted)
    {
        s.Current.Start = now;
        s.FrameStarted = true;
    }
    const uint32_t index = static_cast<uint32_t>(s.Current.Events.size());
    const int32_t parent = s.OpenStack.empty() ? -1 : static_cast<int32_t>(s.OpenStack.back());
    s.Current.Events.push_back({zone, parent, static_cast<int32_t>(s.OpenStack.size()), now, 0});
    s.OpenStack.push_back(index);
    return {s.FrameSerial, index};
}

void EndZone(ZoneToken token)
{
    if (!detail::IsOwnerThread())
        return;
    detail::State& s = detail::Get();
    if (token.FrameSerial != s.FrameSerial || s.OpenStack.empty())
        return; // zone ouverte pendant une image précédente : déjà fermée par NewFrame
    s.Current.Events[token.EventIndex].End = detail::NowNs();
    while (!s.OpenStack.empty())
    {
        const uint32_t top = s.OpenStack.back();
        s.OpenStack.pop_back();
        if (top == token.EventIndex)
            break;
    }
}

void BeginManualZone(ZoneId zone)
{
    if (!detail::IsOwnerThread())
        return;
    detail::Get().ManualStack.push_back(BeginZone(zone));
}

void EndManualZone()
{
    if (!detail::IsOwnerThread())
        return;
    detail::State& s = detail::Get();
    if (s.ManualStack.empty())
        return; // PROFILE_END sans PROFILE_BEGIN correspondant : ignoré
    EndZone(s.ManualStack.back());
    s.ManualStack.pop_back();
}

void SetCounter(CounterId counter, double value)
{
    if (!detail::IsOwnerThread())
        return;
    // Le verrou reste nécessaire : un autre thread peut enregistrer un compteur (et réallouer le tableau) en même
    // temps.
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    s.Counters[counter].Current = value;
}

void NewFrame()
{
    if (!detail::IsOwnerThread())
        return;
    detail::State& s = detail::Get();
    const int64_t now = detail::NowNs();

    if (s.FrameStarted)
    {
        s.Current.End = now;
        if (!s.Paused)
        {
            const std::lock_guard<std::mutex> lock(s.RegistryMutex);
            detail::Commit(s);

            // Mode capture : on fige l'historique sur la première image hors budget.
            const int64_t last = s.FrameCount - 1;
            if (s.PauseOnSpike && s.FrameMs[detail::Slot(last)] > s.BudgetMs)
            {
                s.Paused = true;
                s.PauseOnSpike = false;
                s.SelectedFrame = last;
                s.FollowLatest = false;
            }
        }
    }

    s.Current.Events.clear();
    s.Current.Start = now;
    s.Current.End = 0;
    s.OpenStack.clear();
    s.ManualStack.clear();
    s.FrameStarted = true;
    ++s.FrameSerial;
}

void SetBudgetMs(float budgetMs) { detail::Get().BudgetMs = budgetMs; }

void Reset()
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    detail::ResetLocked(s);
}

ZoneStats GetZoneStats(const char* name, int window)
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    const int w = detail::EffectiveWindow(s, window);

    // Plusieurs zones peuvent porter le même nom (fichiers différents) : leurs temps sont additionnés par image.
    float total[detail::kHistory] = {};
    float calls[detail::kHistory] = {};
    bool found = false;
    for (ZoneId z = 0; z < s.Zones.size(); ++z)
    {
        if (std::strcmp(s.Zones[z].Name, name) != 0)
            continue;
        found = true;
        for (int i = 0; i < detail::kHistory; ++i)
        {
            total[i] += s.ZoneHistories[z].Total[i];
            calls[i] += static_cast<float>(s.ZoneHistories[z].Calls[i]);
        }
    }
    if (!found)
        return {};
    const detail::Summary totalSummary = detail::Summarize(s, total, w);
    const detail::Summary callsSummary = detail::Summarize(s, calls, w);
    return detail::ToStats(totalSummary, callsSummary);
}

ZoneStats GetFrameStats(int window)
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    const detail::Summary frame = detail::Summarize(s, s.FrameMs, detail::EffectiveWindow(s, window));
    detail::Summary one;
    one.Avg = 1.0f;
    return detail::ToStats(frame, one);
}

bool ExportChromeTrace(const char* path)
{
    detail::State& s = detail::Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    return detail::ExportChromeTraceLocked(s, path);
}

} // namespace prof

#endif // PROFILER_IMPLEMENTATION

// =====================================================================================================================
// IMPLÉMENTATION : fenêtre Dear ImGui
// =====================================================================================================================
#if defined(PROFILER_IMGUI_IMPLEMENTATION) && !defined(PROFILER_IMGUI_IMPLEMENTATION_INCLUDED)
#define PROFILER_IMGUI_IMPLEMENTATION_INCLUDED

#include "imgui.h"

#include <cfloat>

namespace prof
{
namespace detail
{
// Teintes réparties par le nombre d'or, saturation et luminosité modérées : lisibles sur fond clair ou sombre.
inline ImU32 ZoneColor(size_t index)
{
    const float hue = std::fmod(0.6f + static_cast<float>(index) * 0.61803398875f, 1.0f);
    return ImColor::HSV(hue, 0.45f, 0.80f);
}

inline const char* ZoneName(const State& s, ZoneId zone) { return zone == kFrameZone ? "Image" : s.Zones[zone].Name; }

inline ImU32 ZoneColorOf(const State&, ZoneId zone)
{ return zone == kFrameZone ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : ZoneColor(zone); }

// Texte foncé ou clair selon la luminance du fond, pour garder le contraste dans le flame graph.
inline ImU32 TextColorOn(ImU32 background)
{
    const float r = static_cast<float>((background >> IM_COL32_R_SHIFT) & 0xFF);
    const float g = static_cast<float>((background >> IM_COL32_G_SHIFT) & 0xFF);
    const float b = static_cast<float>((background >> IM_COL32_B_SHIFT) & 0xFF);
    return 0.299f * r + 0.587f * g + 0.114f * b > 140.0f ? IM_COL32(20, 20, 20, 255) : IM_COL32(245, 245, 245, 255);
}

constexpr ImU32 kWarningColor = IM_COL32(230, 110, 60, 255);

// ---------------------------------------------------------------------------------------------------------------------
// Vue 1 : temps par image (barres + ligne de budget)
// ---------------------------------------------------------------------------------------------------------------------
inline void DrawFrameChart(State& s, float height)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float width = std::max(ImGui::GetContentRegionAvail().x, 50.0f);
    ImGui::InvisibleButton("##frameChart", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();
    const ImVec2 p1(p0.x + width, p0.y + height);

    dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg));

    const int n = static_cast<int>(std::min<int64_t>(kHistory, s.FrameCount));
    if (n == 0)
    {
        dl->AddText(
            ImVec2(p0.x + 6.0f, p0.y + 6.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), "En attente d'images..."
        );
        return;
    }

    // Échelle robuste : le p99 de l'historique, et au moins 1,5 fois le budget.
    s.Scratch.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        s.Scratch[static_cast<size_t>(i)] = s.FrameMs[Slot(s.FrameCount - 1 - i)];
    const size_t p99 = static_cast<size_t>(std::ceil(0.99 * n)) - 1;
    std::nth_element(s.Scratch.begin(), s.Scratch.begin() + static_cast<std::ptrdiff_t>(p99), s.Scratch.end());
    const float scaleMax = std::max(s.Scratch[p99] * 1.1f, s.BudgetMs * 1.5f);

    const float barWidth = width / static_cast<float>(kHistory);
    const ImU32 normal = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    const ImU32 selected = ImGui::GetColorU32(ImGuiCol_Text);

    for (int i = 0; i < n; ++i)
    {
        const int64_t frame = s.FrameCount - 1 - i;
        const float ms = s.FrameMs[Slot(frame)];
        const float x1 = p1.x - static_cast<float>(i) * barWidth;
        const float x0 = x1 - std::max(barWidth - 1.0f, 1.0f);
        const float h = std::min(ms / scaleMax, 1.0f) * height;
        const bool isSelected = !s.FollowLatest && frame == s.SelectedFrame;
        dl->AddRectFilled(
            ImVec2(x0, p1.y - h), ImVec2(x1, p1.y), isSelected ? selected : (ms > s.BudgetMs ? kWarningColor : normal)
        );
        if (ms > scaleMax) // valeur hors échelle : marqueur en haut du graphe
            dl->AddRectFilled(ImVec2(x0, p0.y), ImVec2(x1, p0.y + 3.0f), kWarningColor);
    }

    // Ligne de budget en pointillés, étiquetée directement.
    const float budgetY = p1.y - std::min(s.BudgetMs / scaleMax, 1.0f) * height;
    const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_Text, 0.6f);
    for (float x = p0.x; x < p1.x; x += 8.0f)
        dl->AddLine(ImVec2(x, budgetY), ImVec2(std::min(x + 4.0f, p1.x), budgetY), lineColor);
    char label[32];
    std::snprintf(label, sizeof(label), "budget %.1f ms", static_cast<double>(s.BudgetMs));
    dl->AddText(ImVec2(p0.x + 4.0f, budgetY - ImGui::GetTextLineHeight() - 1.0f), lineColor, label);

    if (hovered)
    {
        const int i = static_cast<int>((p1.x - ImGui::GetIO().MousePos.x) / barWidth);
        if (i >= 0 && i < n)
        {
            const int64_t frame = s.FrameCount - 1 - i;
            const double ms = static_cast<double>(s.FrameMs[Slot(frame)]);
            ImGui::BeginTooltip();
            ImGui::Text("Image #%lld", static_cast<long long>(frame));
            ImGui::Text("%.3f ms  (%.0f FPS)", ms, ms > 0.0 ? 1000.0 / ms : 0.0);
            ImGui::TextDisabled("Clic : inspecter cette image");
            ImGui::EndTooltip();
            if (clicked)
            {
                s.SelectedFrame = frame;
                s.FollowLatest = false;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Vue 2 : flame graph (icicle) d'une image
// ---------------------------------------------------------------------------------------------------------------------
inline void DrawFlameGraph(State& s, const FrameRecord& frame)
{
    const int64_t duration = std::max<int64_t>(frame.End - frame.Start, 1);
    int maxDepth = 0;
    for (const Event& e : frame.Events)
        maxDepth = std::max(maxDepth, static_cast<int>(e.Depth));

    const float rowHeight = ImGui::GetTextLineHeight() + 4.0f;
    const float height = static_cast<float>(maxDepth + 1) * rowHeight + 2.0f;
    const float width = std::max(ImGui::GetContentRegionAvail().x, 50.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + width, p0.y + height);
    ImGui::InvisibleButton("##flame", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg));
    dl->PushClipRect(p0, p1, true);

    auto toX = [&](int64_t t) {
        return p0.x + static_cast<float>(static_cast<double>(t - frame.Start) / static_cast<double>(duration)) * width;
    };

    int hoveredEvent = -1;
    for (size_t i = 0; i < frame.Events.size(); ++i)
    {
        const Event& e = frame.Events[i];
        const float x0 = toX(e.Start);
        const float x1 = std::max(toX(e.End == 0 ? frame.End : e.End), x0 + 1.0f);
        const float y0 = p0.y + static_cast<float>(e.Depth) * rowHeight;
        const float y1 = y0 + rowHeight - 1.0f;

        const ImU32 color = ZoneColor(e.Zone);
        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);

        const char* name = s.Zones[e.Zone].Name;
        if (x1 - x0 > ImGui::CalcTextSize(name).x + 6.0f)
            dl->AddText(ImVec2(x0 + 3.0f, y0 + 2.0f), TextColorOn(color), name);

        if (hovered && mouse.x >= x0 && mouse.x < x1 && mouse.y >= y0 && mouse.y < y1)
            hoveredEvent = static_cast<int>(i);
    }
    dl->PopClipRect();

    if (hoveredEvent >= 0)
    {
        const Event& e = frame.Events[static_cast<size_t>(hoveredEvent)];
        const float total = NsToMs((e.End == 0 ? frame.End : e.End) - e.Start);
        float self = total;
        for (const Event& child : frame.Events)
            if (child.Parent == hoveredEvent)
                self -= NsToMs((child.End == 0 ? frame.End : child.End) - child.Start);

        ImGui::BeginTooltip();
        ImGui::TextUnformatted(s.Zones[e.Zone].Name);
        ImGui::Separator();
        ImGui::Text("Durée  : %.3f ms", static_cast<double>(total));
        ImGui::Text("Propre : %.3f ms", static_cast<double>(self));
        ImGui::Text("Image  : %.1f %%", 100.0 * static_cast<double>(total) / static_cast<double>(NsToMs(duration)));
        ImGui::TextDisabled("%s:%d", s.Zones[e.Zone].File, s.Zones[e.Zone].Line);
        ImGui::EndTooltip();
    }

    ImGui::TextDisabled("0 ms  ->  %.3f ms", static_cast<double>(NsToMs(duration)));
}

// ---------------------------------------------------------------------------------------------------------------------
// Vue 3 : tableau (arbre d'appels ou vue à plat)
// ---------------------------------------------------------------------------------------------------------------------
inline void ColorSwatch(ImU32 color)
{
    const float line = ImGui::GetTextLineHeight();
    const float size = line * 0.6f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float offset = (line - size) * 0.5f;
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(p.x, p.y + offset), ImVec2(p.x + size, p.y + offset + size), color
    );
    ImGui::Dummy(ImVec2(size, line));
    ImGui::SameLine();
}

inline void ShareBar(float fraction)
{
    char text[16];
    std::snprintf(text, sizeof(text), "%.1f %%", 100.0 * static_cast<double>(fraction));
    ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f), text);
}

inline void Sparkline(State& s, const float* history, ImU32 color)
{
    int count = 0;
    const float* data = Chronological(s, history, s.Window, count);
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PlotLines("##trend", data, count, 0, nullptr, 0.0f, FLT_MAX, ImVec2(-1.0f, ImGui::GetTextLineHeight()));
    ImGui::PopStyleColor();
}

inline void StatColumns(
    State& s,
    const Summary& total,
    const Summary& self,
    const Summary& calls,
    float frameAvg,
    const float* history,
    ImU32 color
)
{
    ImGui::TableNextColumn();
    ImGui::Text("%.3f", static_cast<double>(total.Avg));
    ImGui::TableNextColumn();
    if (total.P95 > s.BudgetMs)
        ImGui::TextColored(ImColor(kWarningColor), "%.3f", static_cast<double>(total.P95));
    else
        ImGui::Text("%.3f", static_cast<double>(total.P95));
    ImGui::TableNextColumn();
    ImGui::Text("%.3f", static_cast<double>(total.Max));
    ImGui::TableNextColumn();
    ImGui::Text("%.3f", static_cast<double>(self.Avg));
    ImGui::TableNextColumn();
    ShareBar(frameAvg > 0.0f ? total.Avg / frameAvg : 0.0f);
    ImGui::TableNextColumn();
    ImGui::Text("%.1f", static_cast<double>(calls.Avg));
    ImGui::TableNextColumn();
    Sparkline(s, history, color);
}

inline void SetupColumns()
{
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Zone", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide, 3.0f);
    ImGui::TableSetupColumn(
        "Moy. (ms)",
        ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending |
            ImGuiTableColumnFlags_DefaultSort
    );
    ImGui::TableSetupColumn("P95 (ms)", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn(
        "Propre (ms)", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending
    );
    ImGui::TableSetupColumn("% image", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort, 1.5f);
    ImGui::TableSetupColumn("Appels", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn("Tendance", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort, 1.5f);
    ImGui::TableHeadersRow();
}

inline void DrawNodeRow(State& s, int nodeIndex, float frameAvg)
{
    const Node& node = s.Nodes[static_cast<size_t>(nodeIndex)];
    const Summary total = Summarize(s, node.Total, s.Window);
    if (nodeIndex != 0 && total.Max <= 0.0f)
        return; // zone absente de la fenêtre observée
    const Summary self = Summarize(s, node.Self, s.Window);
    const Summary calls = Summarize(s, node.Calls, s.Window);

    // Enfants triés par temps moyen décroissant : les plus coûteux d'abord.
    std::vector<std::pair<float, int>> children;
    children.reserve(node.Children.size());
    for (int child : node.Children)
        children.emplace_back(Summarize(s, s.Nodes[static_cast<size_t>(child)].Total, s.Window).Avg, child);
    std::sort(children.begin(), children.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(nodeIndex);
    ColorSwatch(ZoneColorOf(s, node.Zone));
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (node.Depth < 2)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    const bool open = ImGui::TreeNodeEx(ZoneName(s, node.Zone), flags);
    if (node.Zone != kFrameZone && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s:%d", s.Zones[node.Zone].File, s.Zones[node.Zone].Line);

    StatColumns(s, total, self, calls, frameAvg, node.Total, ZoneColorOf(s, node.Zone));
    ImGui::PopID();

    if (open && !children.empty())
    {
        for (const auto& child : children)
            DrawNodeRow(s, child.second, frameAvg);
        ImGui::TreePop();
    }
}

inline void DrawTree(State& s, float frameAvg)
{
    const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("##tree", 8, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 14.0f)))
        return;
    SetupColumns();
    DrawNodeRow(s, 0, frameAvg);
    ImGui::EndTable();
}

inline void DrawFlat(State& s, float frameAvg)
{
    const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("##flat", 8, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 14.0f)))
        return;
    SetupColumns();

    struct Row
    {
        ZoneId Zone;
        Summary Total, Self, Calls;
    };
    std::vector<Row> rows;
    for (ZoneId z = 0; z < s.ZoneHistories.size(); ++z)
    {
        const Row row{
            z,
            Summarize(s, s.ZoneHistories[z].Total, s.Window),
            Summarize(s, s.ZoneHistories[z].Self, s.Window),
            Summarize(s, s.ZoneHistories[z].Calls, s.Window)
        };
        if (row.Total.Max > 0.0f)
            rows.push_back(row);
    }

    int sortColumn = 1;
    bool ascending = false;
    if (const ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsCount > 0)
    {
        sortColumn = specs->Specs[0].ColumnIndex;
        ascending = specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
    }
    std::sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
        if (sortColumn == 0)
        {
            const int c = std::strcmp(s.Zones[a.Zone].Name, s.Zones[b.Zone].Name);
            return ascending ? c < 0 : c > 0;
        }
        float va = a.Total.Avg, vb = b.Total.Avg;
        if (sortColumn == 2)
        {
            va = a.Total.P95;
            vb = b.Total.P95;
        }
        if (sortColumn == 3)
        {
            va = a.Total.Max;
            vb = b.Total.Max;
        }
        if (sortColumn == 4)
        {
            va = a.Self.Avg;
            vb = b.Self.Avg;
        }
        if (sortColumn == 6)
        {
            va = a.Calls.Avg;
            vb = b.Calls.Avg;
        }
        return ascending ? va < vb : va > vb;
    });

    for (const Row& row : rows)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(row.Zone));
        ColorSwatch(ZoneColor(row.Zone));
        ImGui::TextUnformatted(s.Zones[row.Zone].Name);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s:%d", s.Zones[row.Zone].File, s.Zones[row.Zone].Line);
        StatColumns(s, row.Total, row.Self, row.Calls, frameAvg, s.ZoneHistories[row.Zone].Total, ZoneColor(row.Zone));
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// ---------------------------------------------------------------------------------------------------------------------
// Vue 4 : valeurs (petites séries temporelles)
// ---------------------------------------------------------------------------------------------------------------------
inline void DrawCounters(State& s)
{
    if (!ImGui::BeginTable("##counters", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
        return;
    ImGui::TableSetupColumn("Valeur", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Dernière", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Évolution (min - max)", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    for (size_t i = 0; i < s.Counters.size(); ++i)
    {
        const Counter& counter = s.Counters[i];
        int count = 0;
        const float* data = Chronological(s, counter.Values, s.Window, count);
        float lo = count > 0 ? *std::min_element(data, data + count) : 0.0f;
        float hi = count > 0 ? *std::max_element(data, data + count) : 0.0f;
        char overlay[64];
        std::snprintf(overlay, sizeof(overlay), "%.6g - %.6g", static_cast<double>(lo), static_cast<double>(hi));
        if (hi - lo < 1e-6f)
        {
            lo -= 0.5f;
            hi += 0.5f;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(counter.Name);
        ImGui::TableNextColumn();
        ImGui::Text("%.6g", count > 0 ? static_cast<double>(data[count - 1]) : 0.0);
        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(i));
        ImGui::PlotLines(
            "##counter", data, count, 0, overlay, lo, hi, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 2.0f)
        );
        ImGui::PopID();
    }
    ImGui::EndTable();
}

} // namespace detail

void DrawWindow(const char* title, bool* open)
{
    using namespace detail;
    State& s = Get();
    const std::lock_guard<std::mutex> lock(s.RegistryMutex);
    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    const Summary frame = Summarize(s, s.FrameMs, s.Window);

    // En-tête : les chiffres clés, avec le p95 et le max à côté de la moyenne.
    ImGui::Text(
        "Image  %.2f ms moy.  |  %.2f ms p95  |  %.2f ms max  |  %.0f FPS",
        static_cast<double>(frame.Avg),
        static_cast<double>(frame.P95),
        static_cast<double>(frame.Max),
        frame.Avg > 0.0f ? 1000.0 / static_cast<double>(frame.Avg) : 0.0
    );

    // Contrôles
    ImGui::Checkbox("Pause", &s.Paused);
    ImGui::SameLine();
    ImGui::Checkbox("Pause au premier pic", &s.PauseOnSpike);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fige l'historique sur la première image qui dépasse le budget, puis se désactive.");
    ImGui::SameLine();
    if (ImGui::Button("Suivre"))
        s.FollowLatest = true;
    ImGui::SameLine();
    if (ImGui::Button("Réinitialiser"))
        ResetLocked(s);

    ImGui::SetNextItemWidth(90.0f);
    const char* budgets[] = {"30 Hz", "60 Hz", "120 Hz", "144 Hz"};
    const float budgetValues[] = {1000.0f / 30.0f, 1000.0f / 60.0f, 1000.0f / 120.0f, 1000.0f / 144.0f};
    int budgetIndex = 1;
    for (int i = 0; i < 4; ++i)
        if (std::abs(budgetValues[i] - s.BudgetMs) < 0.01f)
            budgetIndex = i;
    if (ImGui::Combo("Budget", &budgetIndex, budgets, 4))
        s.BudgetMs = budgetValues[budgetIndex];
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderInt("Fenêtre", &s.Window, 10, kHistory, "%d images");

    // Temps par image
    ImGui::SeparatorText("Temps par image");
    DrawFrameChart(s, 90.0f);

    // Flame graph de l'image suivie ou sélectionnée
    if (s.FrameCount > 0)
    {
        if (s.FollowLatest || s.SelectedFrame < s.FrameCount - kHistory || s.SelectedFrame >= s.FrameCount)
        {
            s.SelectedFrame = s.FrameCount - 1;
            s.FollowLatest = true;
        }
        char header[64];
        std::snprintf(
            header,
            sizeof(header),
            "Image #%lld%s",
            static_cast<long long>(s.SelectedFrame),
            s.FollowLatest ? " (dernière)" : " (sélectionnée)"
        );
        ImGui::SeparatorText(header);
        DrawFlameGraph(s, s.Frames[Slot(s.SelectedFrame)]);
    }

    // Tableau
    ImGui::SeparatorText("Zones");
    if (ImGui::RadioButton("Arbre d'appels", s.TreeView))
        s.TreeView = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("À plat", !s.TreeView))
        s.TreeView = false;
    ImGui::SameLine();
    ImGui::TextDisabled("(sur les %d dernières images)", static_cast<int>(std::min<int64_t>(s.Window, s.FrameCount)));
    if (s.TreeView)
        DrawTree(s, frame.Avg);
    else
        DrawFlat(s, frame.Avg);

    // Valeurs
    if (!s.Counters.empty())
    {
        ImGui::SeparatorText("Valeurs");
        DrawCounters(s);
    }

    // Export
    ImGui::SeparatorText("Export");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputText("##exportPath", s.ExportPath, sizeof(s.ExportPath));
    ImGui::SameLine();
    if (ImGui::Button("Exporter (Chrome Trace)"))
        std::snprintf(
            s.ExportStatus,
            sizeof(s.ExportStatus),
            ExportChromeTraceLocked(s, s.ExportPath) ? "Exporté : ouvrir dans ui.perfetto.dev"
                                                     : "Échec de l'écriture du fichier"
        );
    if (s.ExportStatus[0] != '\0')
        ImGui::TextDisabled("%s", s.ExportStatus);

    ImGui::End();
}
} // namespace prof

#endif // PROFILER_IMGUI_IMPLEMENTATION