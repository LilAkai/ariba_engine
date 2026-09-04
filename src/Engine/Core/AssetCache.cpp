#include "AssetCache.hpp"

#include <SFML/Graphics/Color.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace Core {
namespace {

// Comparaison d'extension insensible à la casse : ".PNG" doit marcher.
[[nodiscard]] std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

template <class T>
[[nodiscard]] const T* Lookup(const AssetMap<T>& map, std::string_view key) noexcept {
    const auto it = FindEntry(map, key);
    return it == map.end() ? nullptr : it->second.get();
}

template <class T>
void EraseKeys(AssetMap<T>& map, const std::vector<std::string>& keys) {
    for (const std::string& key : keys) {
        map.erase(key);
    }
}

}  // namespace

// -----------------------------------------------------------------------------
// Déduction du type et des clés
// -----------------------------------------------------------------------------
AssetCache::AssetType AssetCache::DeduceType(const std::filesystem::path& file) noexcept {
    const std::string extension = ToLower(file.extension().string());

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" ||
        extension == ".tga" || extension == ".gif" || extension == ".psd" || extension == ".hdr" ||
        extension == ".pic") {
        // Un fichier posé dans un dossier nommé "Images" reste en RAM (accès
        // pixel, masque de collision) ; partout ailleurs il part en VRAM.
        const std::filesystem::path parent = file.parent_path().filename();
        return ToLower(parent.string()) == "images" ? AssetType::Image : AssetType::Texture;
    }
    if (extension == ".ttf" || extension == ".otf" || extension == ".ttc" || extension == ".pfb" ||
        extension == ".pfa" || extension == ".cff" || extension == ".woff") {
        return AssetType::Font;
    }
    if (extension == ".wav" || extension == ".ogg" || extension == ".flac" || extension == ".mp3" ||
        extension == ".aiff" || extension == ".au") {
        // Les longs morceaux vont dans un dossier "Music" et sont streamés ;
        // le reste est décompressé en mémoire pour être joué sans latence.
        const std::filesystem::path parent = file.parent_path().filename();
        const std::string           folder = ToLower(parent.string());
        return (folder == "music" || folder == "musics" || folder == "musique") ? AssetType::Music
                                                                               : AssetType::SoundBuffer;
    }
    if (extension == ".vert" || extension == ".vs") {
        return AssetType::VertexShader;
    }
    if (extension == ".frag" || extension == ".fs" || extension == ".glsl") {
        return AssetType::FragmentShader;
    }
    if (extension == ".geom" || extension == ".gs") {
        return AssetType::GeometryShader;
    }
    return AssetType::Unknown;
}

std::string AssetCache::MakeKey(const std::filesystem::path& file) const {
    std::error_code       error;
    std::filesystem::path relative = std::filesystem::relative(file, m_Root, error);
    if (error || relative.empty()) {
        relative = file.filename();
    }
    relative.replace_extension();

    std::string key = relative.generic_string();   // toujours des '/', y compris sous Windows
    return key;
}

// -----------------------------------------------------------------------------
// Groupes
// -----------------------------------------------------------------------------
std::size_t AssetCache::AcquireGroup(std::string_view group) {
    const auto it = m_Groups.try_emplace(std::string(group)).first;
    Group& entry = it->second;

    ++entry.RefCount;
    if (entry.RefCount > 1) {
        return 0;   // déjà chargé par quelqu'un d'autre
    }

    const std::filesystem::path directory = m_Root / std::filesystem::path(group);

    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        ReportError(directory, "dossier de groupe introuvable");
        m_Groups.erase(it);   // pas d'entrée fantôme pour un groupe qui n'existe pas
        return 0;
    }
    return LoadDirectory(directory, entry);
}

bool AssetCache::ReleaseGroup(std::string_view group) {
    Group* entry = FindGroup(group);
    if (entry == nullptr || entry->RefCount == 0) {
        return false;
    }

    --entry->RefCount;
    if (entry->RefCount > 0) {
        return false;
    }

    Unload(*entry);
    m_Groups.erase(std::string(group));
    return true;
}

void AssetCache::ForceUnloadGroup(std::string_view group) {
    Group* entry = FindGroup(group);
    if (entry == nullptr) {
        return;
    }
    Unload(*entry);
    m_Groups.erase(std::string(group));
}

void AssetCache::UnloadAll() {
    m_Textures.clear();
    m_Fonts.clear();
    m_SoundBuffers.clear();
    m_Images.clear();
    m_Shaders.clear();
    m_Musics.clear();
    m_Groups.clear();
}

bool AssetCache::IsGroupLoaded(std::string_view group) const noexcept {
    return FindEntry(m_Groups, group) != m_Groups.end();
}

std::size_t AssetCache::GetGroupRefCount(std::string_view group) const noexcept {
    const auto it = FindEntry(m_Groups, group);
    return it == m_Groups.end() ? 0 : it->second.RefCount;
}

std::vector<std::string> AssetCache::GetLoadedGroups() const {
    std::vector<std::string> groups;
    groups.reserve(m_Groups.size());
    for (const auto& [name, entry] : m_Groups) {
        groups.push_back(name);
    }
    std::sort(groups.begin(), groups.end());
    return groups;
}

std::vector<std::string> AssetCache::ScanAvailableGroups() const {
    std::vector<std::string> groups;

    std::error_code error;
    if (!std::filesystem::is_directory(m_Root, error)) {
        return groups;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_Root, error)) {
        if (entry.is_directory(error)) {
            groups.push_back(entry.path().filename().generic_string());
        }
    }
    std::sort(groups.begin(), groups.end());
    return groups;
}

AssetCache::Group* AssetCache::FindGroup(std::string_view group) noexcept {
    const auto it = FindEntry(m_Groups, group);
    return it == m_Groups.end() ? nullptr : &it->second;
}

// -----------------------------------------------------------------------------
// Chargement
// -----------------------------------------------------------------------------
std::size_t AssetCache::LoadDirectory(const std::filesystem::path& directory, Group& group) {
    std::size_t     loaded = 0;
    std::error_code error;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, error)) {
        if (!entry.is_regular_file(error)) {
            continue;
        }

        const std::filesystem::path& file = entry.path();
        const AssetType              type = DeduceType(file);
        if (type == AssetType::Unknown) {
            continue;   // README, .tmx, .json... ce n'est pas notre affaire
        }

        if (LoadFile(file, MakeKey(file), type, group)) {
            ++loaded;
        }
    }

    if (error) {
        ReportError(directory, "parcours du dossier interrompu");
    }
    return loaded;
}

bool AssetCache::LoadFile(const std::filesystem::path& file, std::string_view key, AssetType type, Group& group) {
    switch (type) {
        case AssetType::Texture: {
            auto texture = std::make_unique<sf::Texture>();
            if (!texture->loadFromFile(file)) {
                ReportError(file, "texture illisible");
                return false;
            }
            texture->setSmooth(SmoothTextures);
            texture->setRepeated(RepeatTextures);
            group.TextureKeys.emplace_back(key);
            m_Textures.insert_or_assign(std::string(key), std::move(texture));
            return true;
        }
        case AssetType::Font: {
            auto font = std::make_unique<sf::Font>();
            if (!font->openFromFile(file)) {
                ReportError(file, "police illisible");
                return false;
            }
            group.FontKeys.emplace_back(key);
            m_Fonts.insert_or_assign(std::string(key), std::move(font));
            return true;
        }
        case AssetType::SoundBuffer: {
            auto buffer = std::make_unique<sf::SoundBuffer>();
            if (!buffer->loadFromFile(file)) {
                ReportError(file, "son illisible");
                return false;
            }
            group.SoundKeys.emplace_back(key);
            m_SoundBuffers.insert_or_assign(std::string(key), std::move(buffer));
            return true;
        }
        case AssetType::Image: {
            auto image = std::make_unique<sf::Image>();
            if (!image->loadFromFile(file)) {
                ReportError(file, "image illisible");
                return false;
            }
            group.ImageKeys.emplace_back(key);
            m_Images.insert_or_assign(std::string(key), std::move(image));
            return true;
        }
        case AssetType::Music: {
            auto music = std::make_unique<sf::Music>();
            if (!music->openFromFile(file)) {
                ReportError(file, "musique illisible");
                return false;
            }
            group.MusicKeys.emplace_back(key);
            m_Musics.insert_or_assign(std::string(key), std::move(music));
            return true;
        }
        case AssetType::VertexShader:
        case AssetType::FragmentShader:
        case AssetType::GeometryShader: {
            if (!sf::Shader::isAvailable()) {
                ReportError(file, "shaders non supportés par le pilote");
                return false;
            }
            const sf::Shader::Type shaderType = (type == AssetType::VertexShader)   ? sf::Shader::Type::Vertex
                                                : (type == AssetType::GeometryShader) ? sf::Shader::Type::Geometry
                                                                                      : sf::Shader::Type::Fragment;
            auto shader = std::make_unique<sf::Shader>();
            if (!shader->loadFromFile(file, shaderType)) {
                ReportError(file, "shader non compilé");
                return false;
            }
            group.ShaderKeys.emplace_back(key);
            m_Shaders.insert_or_assign(std::string(key), std::move(shader));
            return true;
        }
        case AssetType::Unknown:
        default:
            return false;
    }
}

void AssetCache::Unload(Group& group) {
    EraseKeys(m_Textures, group.TextureKeys);
    EraseKeys(m_Fonts, group.FontKeys);
    EraseKeys(m_SoundBuffers, group.SoundKeys);
    EraseKeys(m_Images, group.ImageKeys);
    EraseKeys(m_Shaders, group.ShaderKeys);
    EraseKeys(m_Musics, group.MusicKeys);

    group = Group{};
}

// -----------------------------------------------------------------------------
// Chargement à l'unité
// -----------------------------------------------------------------------------
bool AssetCache::LoadTexture(std::string_view key, const std::filesystem::path& file, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);
    return LoadFile(file, key, AssetType::Texture, entry);
}

bool AssetCache::LoadFont(std::string_view key, const std::filesystem::path& file, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);
    return LoadFile(file, key, AssetType::Font, entry);
}

bool AssetCache::LoadSoundBuffer(std::string_view key, const std::filesystem::path& file, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);
    return LoadFile(file, key, AssetType::SoundBuffer, entry);
}

bool AssetCache::LoadImage(std::string_view key, const std::filesystem::path& file, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);
    return LoadFile(file, key, AssetType::Image, entry);
}

bool AssetCache::LoadMusic(std::string_view key, const std::filesystem::path& file, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);
    return LoadFile(file, key, AssetType::Music, entry);
}

bool AssetCache::LoadShader(std::string_view key, const std::filesystem::path& file, sf::Shader::Type type,
                            std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);

    if (!sf::Shader::isAvailable()) {
        ReportError(file, "shaders non supportés par le pilote");
        return false;
    }

    auto shader = std::make_unique<sf::Shader>();
    if (!shader->loadFromFile(file, type)) {
        ReportError(file, "shader non compilé");
        return false;
    }
    entry.ShaderKeys.emplace_back(key);
    m_Shaders.insert_or_assign(std::string(key), std::move(shader));
    return true;
}

bool AssetCache::LoadShader(std::string_view key, const std::filesystem::path& vertexFile,
                            const std::filesystem::path& fragmentFile, std::string_view group) {
    Group& entry = m_Groups.try_emplace(std::string(group)).first->second;
    entry.RefCount = std::max<std::size_t>(entry.RefCount, 1);

    if (!sf::Shader::isAvailable()) {
        ReportError(vertexFile, "shaders non supportés par le pilote");
        return false;
    }

    auto shader = std::make_unique<sf::Shader>();
    if (!shader->loadFromFile(vertexFile, fragmentFile)) {
        ReportError(vertexFile, "programme de shader non lié");
        return false;
    }
    entry.ShaderKeys.emplace_back(key);
    m_Shaders.insert_or_assign(std::string(key), std::move(shader));
    return true;
}

// -----------------------------------------------------------------------------
// Accès
// -----------------------------------------------------------------------------
const sf::Texture* AssetCache::TryGetTexture(std::string_view key) const noexcept {
    return Lookup(m_Textures, key);
}

const sf::Font* AssetCache::TryGetFont(std::string_view key) const noexcept {
    return Lookup(m_Fonts, key);
}

const sf::SoundBuffer* AssetCache::TryGetSoundBuffer(std::string_view key) const noexcept {
    return Lookup(m_SoundBuffers, key);
}

const sf::Image* AssetCache::TryGetImage(std::string_view key) const noexcept {
    return Lookup(m_Images, key);
}

sf::Shader* AssetCache::TryGetShader(std::string_view key) noexcept {
    return const_cast<sf::Shader*>(Lookup(m_Shaders, key));
}

sf::Music* AssetCache::TryGetMusic(std::string_view key) noexcept {
    return const_cast<sf::Music*>(Lookup(m_Musics, key));
}

const sf::Texture& AssetCache::GetTexture(std::string_view key) const {
    if (const sf::Texture* texture = TryGetTexture(key)) {
        return *texture;
    }
    if (UsePlaceholderTexture) {
        return GetPlaceholderTexture();
    }
    throw std::runtime_error("AssetCache : texture absente : " + std::string(key));
}

const sf::Font& AssetCache::GetFont(std::string_view key) const {
    if (const sf::Font* font = TryGetFont(key)) {
        return *font;
    }
    throw std::runtime_error("AssetCache : police absente : " + std::string(key));
}

const sf::SoundBuffer& AssetCache::GetSoundBuffer(std::string_view key) const {
    if (const sf::SoundBuffer* buffer = TryGetSoundBuffer(key)) {
        return *buffer;
    }
    throw std::runtime_error("AssetCache : son absent : " + std::string(key));
}

const sf::Image& AssetCache::GetImage(std::string_view key) const {
    if (const sf::Image* image = TryGetImage(key)) {
        return *image;
    }
    throw std::runtime_error("AssetCache : image absente : " + std::string(key));
}

const sf::Shader& AssetCache::GetShader(std::string_view key) const {
    if (const sf::Shader* shader = Lookup(m_Shaders, key)) {
        return *shader;
    }
    throw std::runtime_error("AssetCache : shader absent : " + std::string(key));
}

sf::Shader& AssetCache::GetShader(std::string_view key) {
    if (sf::Shader* shader = TryGetShader(key)) {
        return *shader;
    }
    throw std::runtime_error("AssetCache : shader absent : " + std::string(key));
}

sf::Music& AssetCache::GetMusic(std::string_view key) {
    if (sf::Music* music = TryGetMusic(key)) {
        return *music;
    }
    throw std::runtime_error("AssetCache : musique absente : " + std::string(key));
}

// Damier magenta et noir : impossible à confondre avec une vraie texture.
const sf::Texture& AssetCache::GetPlaceholderTexture() const {
    if (!m_Placeholder) {
        constexpr unsigned int size = 64;
        constexpr unsigned int cell = 8;

        sf::Image image(sf::Vector2u{size, size}, sf::Color::Black);
        for (unsigned int y = 0; y < size; ++y) {
            for (unsigned int x = 0; x < size; ++x) {
                const bool magenta = ((x / cell) + (y / cell)) % 2 == 0;
                if (magenta) {
                    image.setPixel(sf::Vector2u{x, y}, sf::Color::Magenta);
                }
            }
        }

        m_Placeholder = std::make_unique<sf::Texture>();
        (void)m_Placeholder->loadFromImage(image);
        m_Placeholder->setRepeated(true);
    }
    return *m_Placeholder;
}

// -----------------------------------------------------------------------------
// Diagnostic
// -----------------------------------------------------------------------------
AssetCache::Stats AssetCache::GetStats() const noexcept {
    Stats stats;
    stats.Textures     = m_Textures.size();
    stats.Fonts        = m_Fonts.size();
    stats.SoundBuffers = m_SoundBuffers.size();
    stats.Images       = m_Images.size();
    stats.Shaders      = m_Shaders.size();
    stats.Musics       = m_Musics.size();
    stats.Groups       = m_Groups.size();

    for (const auto& [key, texture] : m_Textures) {
        const sf::Vector2u size = texture->getSize();
        stats.TextureBytes += static_cast<std::size_t>(size.x) * size.y * 4;
    }
    for (const auto& [key, buffer] : m_SoundBuffers) {
        stats.SoundBytes += static_cast<std::size_t>(buffer->getSampleCount()) * 2;
    }
    return stats;
}

void AssetCache::ReportError(const std::filesystem::path& file, std::string_view reason) {
    m_Errors.push_back(file.generic_string() + " : " + std::string(reason));
}

}  // namespace Core
