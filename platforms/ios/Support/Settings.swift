import Foundation
import Security
import RimesCore

func L(_ zh: String, _ en: String) -> String { Locale.preferredLanguages.first?.hasPrefix("zh") == true ? zh : en }

struct AppConfiguration: Codable {
    var scheme: InputScheme = .pinyin
    var schemeSelectionRevision: UUID?
    var chord = ChordProfile.builtIn
    /// Keep the persisted mapping format intact; the default keyboard uses the
    /// same pinyin-to-Ziranma encoding layer as desktop Ziranma chord profiles.
    var keyboardChord: ChordProfile {
        var profile = chord
        if profile.id == ChordProfile.builtIn.id { profile.outputEncoding = .ziranma }
        return profile
    }
    var profiles: [ChordProfile] = []
    var providers: [ProviderConfiguration] = []
    var selectedProvider: UUID?
    var consents: [String] = []
    var translationLanguage = "English"
    var provider: ProviderConfiguration? { providers.first { $0.id == selectedProvider } }
}
/// Only the containing app writes this snapshot. The keyboard never requires group write access.
final class ConfigurationStore: ConfigurationStorage {
    static var groupID: String { Bundle.main.object(forInfoDictionaryKey: "RIMESAppGroup") as? String ?? "group.org.scholay.rimes.ios" }
    private var root: URL {
        FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: Self.groupID)
            ?? FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
    }
    private var file: URL { root.appendingPathComponent("configuration-v1.json") }
    func load() -> AppConfiguration {
        guard let data = try? Data(contentsOf: file), data.count < 8 * 1024 * 1024,
              var value = try? JSONDecoder().decode(AppConfiguration.self, from: data),
              value.providers.count <= 32, value.profiles.count <= 128 else { return AppConfiguration() }
        value.chord = (try? value.chord.validated()) ?? .builtIn
        value.profiles = value.profiles.compactMap { try? $0.validated() }
        return value
    }
    func save(_ value: AppConfiguration) throws {
        _ = try value.chord.validated()
        guard value.providers.count <= 32, value.profiles.count <= 128 else { throw CoreError.tooLarge }
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        try JSONEncoder().encode(value).write(to: file, options: [.atomic, .completeFileProtection])
        var excluded = root; var values = URLResourceValues(); values.isExcludedFromBackup = true; try excluded.setResourceValues(values)
    }
}
final class KeychainStore: ProviderSecretStore {
    /// The upstream keychain group is stamped as `$(AppIdentifierPrefix)org.scholay.rimes.ios.shared`.
    /// Developer-signed builds resolve the prefix, so the shared group is usable and the keyboard
    /// extension can read secrets. Sideloading re-signs entitlements to the installer's team while
    /// the Info.plist value stays unprefixed, and querying an access group that is absent from the
    /// signed entitlements fails every SecItem call with errSecMissingEntitlement (-34018).
    /// Resolution: try the shared group first; on -34018 retry without an access group, which
    /// always succeeds in the containing app (the keyboard then simply cannot read the secret).
    private func query(_ id: UUID) -> [String: Any] {
        var q: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: "org.scholay.rimes.ios.providers", kSecAttrAccount as String: id.uuidString,
            kSecAttrSynchronizable as String: false]
        if let group = Self.resolvedAccessGroup { q[kSecAttrAccessGroup as String] = group }
        return q
    }
    /// The shared keychain access group, resolved once per process. The Info.plist template value
    /// carries the literal `$(AppIdentifierPrefix)`; signed builds substitute the team identifier.
    /// On a re-signed sideload the value stays unprefixed, so the prefix is recovered from the
    /// embedded provisioning profile, letting the keyboard extension read app-saved secrets.
    /// Returning nil means "use the app's default access group" instead of failing with -34018.
    static let resolvedAccessGroup: String? = {
        guard let group = Bundle.main.object(forInfoDictionaryKey: "RIMESKeychainGroup") as? String,
              !group.isEmpty, !group.hasPrefix("$(") else { return nil }
        if group.contains(".") && !group.hasPrefix(".") { return group }   // already TeamID-prefixed
        guard let path = Bundle.main.path(forResource: "embedded", ofType: "mobileprovision"),
              let data = try? Data(contentsOf: URL(fileURLWithPath: path), options: .mappedIfSafe),
              // The profile is a DER/CMS blob; latin-1 decodes any byte sequence losslessly,
              // unlike UTF-8 which returns nil on the binary header.
              let text = String(data: data, encoding: .isoLatin1),
              let range = text.range(of: "<key>TeamIdentifier</key>[^<]*<string>([A-Za-z0-9]{10})</string>",
                                     options: .regularExpression) else { return nil }
        let match = String(text[range])
        guard let open = match.range(of: "<string>")?.upperBound,
              let close = match.range(of: "</string>", range: open..<match.endIndex)?.lowerBound else { return nil }
        let team = String(match[open..<close])
        return team.isEmpty ? nil : "\(team).\(group)"
    }()
    private func withoutGroup(_ q: [String: Any]) -> [String: Any] {
        var plain = q; plain.removeValue(forKey: kSecAttrAccessGroup as String); return plain
    }
    func read(_ id: UUID) throws -> String {
        var q = query(id); q[kSecReturnData as String] = true; q[kSecMatchLimit as String] = kSecMatchLimitOne
        var object: CFTypeRef?
        var status = SecItemCopyMatching(q as CFDictionary, &object)
        if status == errSecMissingEntitlement { q = withoutGroup(q); status = SecItemCopyMatching(q as CFDictionary, &object) }
        // A secret written through the no-group fallback is invisible to a group-scoped lookup on
        // platforms that answer with not-found instead of -34018; check the bare query too.
        if status == errSecItemNotFound, q[kSecAttrAccessGroup as String] != nil {
            let plain = withoutGroup(q)
            let retry = SecItemCopyMatching(plain as CFDictionary, &object)
            if retry == errSecSuccess { status = retry }
        }
        if status == errSecItemNotFound { return "" }
        guard status == errSecSuccess, let data = object as? Data, let key = String(data: data, encoding: .utf8) else { throw NSError(domain: NSOSStatusErrorDomain, code: Int(status)) }
        return key
    }
    func save(_ key: String, id: UUID) throws {
        let attributes: [String: Any] = [kSecValueData as String: Data(key.utf8), kSecAttrAccessible as String: kSecAttrAccessibleWhenUnlockedThisDeviceOnly]
        var q = query(id)
        var status = SecItemUpdate(q as CFDictionary, attributes as CFDictionary)
        if status == errSecMissingEntitlement {
            q = withoutGroup(q)
            status = SecItemUpdate(q as CFDictionary, attributes as CFDictionary)
        }
        if status == errSecItemNotFound {
            var entry = q; attributes.forEach { entry[$0.key] = $0.value }
            status = SecItemAdd(entry as CFDictionary, nil)
            // Add with the shared group can also hit the missing entitlement; retry bare.
            if status == errSecMissingEntitlement { entry = withoutGroup(entry); status = SecItemAdd(entry as CFDictionary, nil) }
        }
        guard status == errSecSuccess else { throw NSError(domain: NSOSStatusErrorDomain, code: Int(status)) }
    }
    func delete(_ id: UUID) throws {
        var q = query(id)
        var status = SecItemDelete(q as CFDictionary)
        if status == errSecMissingEntitlement { q = withoutGroup(q); status = SecItemDelete(q as CFDictionary) }
        guard status == errSecSuccess || status == errSecItemNotFound else { throw NSError(domain: NSOSStatusErrorDomain, code: Int(status)) }
    }
}

final class KeyboardPreferenceStore {
    private let defaults: UserDefaults
    init(defaults: UserDefaults = .standard) { self.defaults = defaults }
    func load() -> KeyboardPreferences {
        guard let data = defaults.data(forKey: "keyboard-preferences-v1"),
              let value = try? JSONDecoder().decode(KeyboardPreferences.self, from: data) else { return .init() }
        return value
    }
    func save(_ value: KeyboardPreferences) {
        if let data = try? JSONEncoder().encode(value) { defaults.set(data, forKey: "keyboard-preferences-v1") }
    }
}
