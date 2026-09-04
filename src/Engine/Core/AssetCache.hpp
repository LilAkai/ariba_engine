// =============================================================================
//  AssetCache.hpp
//  Cache de ressources SFML 3.0.2, organisé en groupes chargés/déchargés à la
//  demande par les états.
//
//  ARBORESCENCE ATTENDUE
//  ---------------------
//  Un sous-dossier de Resources/ = un groupe. Le type est déduit de l'extension.
//
//      Resources/
//          Common/          <- police d'UI, sons de clic : chargé une fois pour toutes
//              Fonts/main.ttf
//              Sfx/click.wav
//          Menu/
//              Textures/background.png
//              Textures/logo.png
//              Music/theme.ogg
//          Game/
//              Textures/player.png
//              Textures/tileset.png
//              Shaders/water.frag
//
//  La clé d'une ressource est son chemin relatif à Resources/, sans extension :
//  "Menu/Textures/background", "Common/Fonts/main". Explicite, sans collision
//  possible entre deux "background.png" de groupes différents.
//
//  UTILISATION
//  -----------
//      // au démarrage, dans Application
//      m_Assets.SetRoot("Resources");
//      m_Assets.AcquireGroup("Common");          // jamais relâché
//
//      // dans MenuState
//      void OnEnter() override {
//          m_Scope = Core::AssetScope(GetContext().GetAssets(), "Menu");
//          m_Sprite.setTexture(GetContext().GetAssets().GetTexture("Menu/Textures/background"));
//      }
//      // rien à écrire dans OnExit : m_Scope relâche le groupe en se détruisant,
//      // et les textures du menu quittent la VRAM avant que GameState ne charge
//      // les siennes.
//
//  COMPTAGE DE RÉFÉRENCES
//  ----------------------
//  Un groupe est chargé au premier AcquireGroup et déchargé quand le dernier
//  utilisateur le relâche. Deux états qui partagent "Common" ne se marchent
//  donc pas dessus, et un aller-retour Menu -> Pause -> Menu ne recharge rien
//  tant qu'un état tient encore le groupe.
//
//  ATTENTION : après ReleaseGroup, toute sf::Sprite / sf::Text qui pointait
//  encore sur une ressource du groupe est pendante. Relâche dans OnExit, une
//  fois que plus personne ne dessine l'état.
// =============================================================================
#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Core {

// -----------------------------------------------------------------------------
// Hachage transparent : permet Get("Menu/Textures/logo") avec un std::string_view
// ou un littéral sans construire de std::string temporaire à chaque appel.
// -----------------------------------------------------------------------------
struct StringHash {
    using is_transparent = void;

    [[nodiscard]] std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

// La recherche hétérogène sur unordered_map n'existe qu'à partir de C++20 :
// en dessous, on retombe sur une map classique et Get() construit un
// std::string temporaire. Même code, une allocation de plus par appel.
#if defined(__cpp_lib_generic_unordered_lookup) && __cpp_lib_generic_unordered_lookup >= 201811L
#define CORE_TRANSPARENT_LOOKUP 1
template <class V>
using KeyMap = std::unordered_map<std::string, V, StringHash, std::equal_to<>>;
#else
template <class V>
using KeyMap = std::unordered_map<std::string, V>;
#endif

template <class T>
using AssetMap = KeyMap<std::unique_ptr<T>>;

template <class Map>
[[nodiscard]] auto FindEntry(Map& map, std::string_view key) {
#ifdef CORE_TRANSPARENT_LOOKUP
    return map.find(key);
#else
    return map.find(std::string(key));
#endif
}

// -----------------------------------------------------------------------------
class AssetCache {
public:
    AssetCache() = default;
    ~AssetCache() = default;

    AssetCache(const AssetCache&)            = delete;
    AssetCache& operator=(const AssetCache&) = delete;

    // --- Configuration ------------------------------------------------------
    void SetRoot(const std::filesystem::path& root) { m_Root = root; }
    [[nodiscard]] const std::filesystem::path& GetRoot() const noexcept { return m_Root; }

    // Appliqués à chaque texture chargée ensuite.
    bool SmoothTextures = true;
    bool RepeatTextures = false;

    // Une texture manquante renvoie un damier magenta au lieu de lever une
    // exception : le jeu tourne, le trou se voit à l'écran.
    bool UsePlaceholderTexture = true;

    // --- Groupes ------------------------------------------------------------
    // Charge Resources/<group> récursivement au premier appel, incrémente le
    // compteur ensuite. Renvoie le nombre de fichiers chargés (0 si déjà là).
    std::size_t AcquireGroup(std::string_view group);

    // Décrémente ; libère réellement à zéro. Renvoie true si le groupe a été
    // déchargé par cet appel.
    bool ReleaseGroup(std::string_view group);

    // Force le déchargement, quel que soit le compteur. À réserver aux
    // rechargements à chaud pendant le développement.
    void ForceUnloadGroup(std::string_view group);

    void UnloadAll();

    [[nodiscard]] bool        IsGroupLoaded(std::string_view group) const noexcept;
    [[nodiscard]] std::size_t GetGroupRefCount(std::string_view group) const noexcept;
    [[nodiscard]] std::vector<std::string> GetLoadedGroups() const;

    // Liste les sous-dossiers de Resources/ sans rien charger.
    [[nodiscard]] std::vector<std::string> ScanAvailableGroups() const;

    // --- Accès --------------------------------------------------------------
    // Get* : la ressource DOIT être là (groupe acquis). Sinon exception, ou
    // damier pour les textures si UsePlaceholderTexture.
    [[nodiscard]] const sf::Texture&     GetTexture(std::string_view key) const;
    [[nodiscard]] const sf::Font&        GetFont(std::string_view key) const;
    [[nodiscard]] const sf::SoundBuffer& GetSoundBuffer(std::string_view key) const;
    [[nodiscard]] const sf::Image&       GetImage(std::string_view key) const;
    [[nodiscard]] const sf::Shader&      GetShader(std::string_view key) const;

    // Le shader et la musique doivent être modifiables à l'usage (setUniform,
    // play), d'où les surcharges non const.
    [[nodiscard]] sf::Shader& GetShader(std::string_view key);
    [[nodiscard]] sf::Music&  GetMusic(std::string_view key);

    // Try* : nullptr si absent, aucune exception.
    [[nodiscard]] const sf::Texture*     TryGetTexture(std::string_view key) const noexcept;
    [[nodiscard]] const sf::Font*        TryGetFont(std::string_view key) const noexcept;
    [[nodiscard]] const sf::SoundBuffer* TryGetSoundBuffer(std::string_view key) const noexcept;
    [[nodiscard]] const sf::Image*       TryGetImage(std::string_view key) const noexcept;
    [[nodiscard]] sf::Shader*            TryGetShader(std::string_view key) noexcept;
    [[nodiscard]] sf::Music*             TryGetMusic(std::string_view key) noexcept;

    // --- Chargement à l'unité -----------------------------------------------
    // Pour un fichier isolé hors arborescence de groupes. Rattaché au groupe
    // indiqué, donc soumis au même comptage de références.
    bool LoadTexture(std::string_view key, const std::filesystem::path& file, std::string_view group);
    bool LoadFont(std::string_view key, const std::filesystem::path& file, std::string_view group);
    bool LoadSoundBuffer(std::string_view key, const std::filesystem::path& file, std::string_view group);
    bool LoadImage(std::string_view key, const std::filesystem::path& file, std::string_view group);
    bool LoadMusic(std::string_view key, const std::filesystem::path& file, std::string_view group);
    bool LoadShader(std::string_view key, const std::filesystem::path& file, sf::Shader::Type type,
                    std::string_view group);
    bool LoadShader(std::string_view key, const std::filesystem::path& vertexFile,
                    const std::filesystem::path& fragmentFile, std::string_view group);

    // --- Diagnostic ---------------------------------------------------------
    struct Stats {
        std::size_t Textures      = 0;
        std::size_t Fonts         = 0;
        std::size_t SoundBuffers  = 0;
        std::size_t Images        = 0;
        std::size_t Shaders       = 0;
        std::size_t Musics        = 0;
        std::size_t Groups        = 0;
        std::size_t TextureBytes  = 0;   // estimation VRAM : largeur * hauteur * 4
        std::size_t SoundBytes    = 0;   // estimation RAM : échantillons * 2 octets
    };

    [[nodiscard]] Stats GetStats() const noexcept;

    // Fichiers dont le chargement a échoué depuis le dernier ClearErrors().
    [[nodiscard]] const std::vector<std::string>& GetErrors() const noexcept { return m_Errors; }
    void ClearErrors() { m_Errors.clear(); }

private:
    enum class AssetType : std::uint8_t {
        Unknown,
        Texture,
        Font,
        SoundBuffer,
        Image,
        Music,
        VertexShader,
        FragmentShader,
        GeometryShader
    };

    struct Group {
        std::size_t              RefCount = 0;
        std::vector<std::string> TextureKeys;
        std::vector<std::string> FontKeys;
        std::vector<std::string> SoundKeys;
        std::vector<std::string> ImageKeys;
        std::vector<std::string> ShaderKeys;
        std::vector<std::string> MusicKeys;
    };

    [[nodiscard]] static AssetType DeduceType(const std::filesystem::path& file) noexcept;
    [[nodiscard]] std::string MakeKey(const std::filesystem::path& file) const;

    std::size_t LoadDirectory(const std::filesystem::path& directory, Group& group);
    bool        LoadFile(const std::filesystem::path& file, std::string_view key, AssetType type, Group& group);
    void        Unload(Group& group);

    [[nodiscard]] Group* FindGroup(std::string_view group) noexcept;

    void ReportError(const std::filesystem::path& file, std::string_view reason);

    [[nodiscard]] const sf::Texture& GetPlaceholderTexture() const;

    std::filesystem::path m_Root{"Resources"};

    AssetMap<sf::Texture>     m_Textures;
    AssetMap<sf::Font>        m_Fonts;
    AssetMap<sf::SoundBuffer> m_SoundBuffers;
    AssetMap<sf::Image>       m_Images;
    AssetMap<sf::Shader>      m_Shaders;
    AssetMap<sf::Music>       m_Musics;

    KeyMap<Group> m_Groups;

    std::vector<std::string> m_Errors;

    mutable std::unique_ptr<sf::Texture> m_Placeholder;   // créé au premier besoin
};

// -----------------------------------------------------------------------------
// AssetScope : acquiert un groupe à la construction, le relâche à la
// destruction. Un état en garde un en membre et n'a plus rien à gérer, même
// si OnExit n'est jamais atteint (exception, destruction du manager).
//
//      class MenuState final : public Core::State {
//          void OnEnter() override { m_Scope = Core::AssetScope(GetContext().GetAssets(), "Menu"); }
//      private:
//          Core::AssetScope m_Scope;
//      };
// -----------------------------------------------------------------------------
class AssetScope {
public:
    AssetScope() noexcept = default;

    AssetScope(AssetCache& cache, std::string_view group)
        : m_Cache(&cache)
        , m_Group(group) {
        m_Cache->AcquireGroup(m_Group);
    }

    AssetScope(const AssetScope&)            = delete;
    AssetScope& operator=(const AssetScope&) = delete;

    AssetScope(AssetScope&& other) noexcept
        : m_Cache(other.m_Cache)
        , m_Group(std::move(other.m_Group)) {
        other.m_Cache = nullptr;
    }

    AssetScope& operator=(AssetScope&& other) noexcept {
        if (this != &other) {
            Release();
            m_Cache       = other.m_Cache;
            m_Group       = std::move(other.m_Group);
            other.m_Cache = nullptr;
        }
        return *this;
    }

    ~AssetScope() { Release(); }

    void Release() noexcept {
        if (m_Cache != nullptr) {
            m_Cache->ReleaseGroup(m_Group);
            m_Cache = nullptr;
        }
    }

    [[nodiscard]] bool IsActive() const noexcept { return m_Cache != nullptr; }
    [[nodiscard]] const std::string& GetGroup() const noexcept { return m_Group; }

private:
    AssetCache* m_Cache = nullptr;
    std::string m_Group;
};

}  // namespace Core
