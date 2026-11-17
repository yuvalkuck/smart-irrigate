package org.magicat.irrigate.connect.model

import org.json.JSONObject

/** A single editable value from the device's status JSON, tagged with its original JSON type. */
sealed class ConfigValue {
    data class Text(val value: String) : ConfigValue()
    data class Number(val value: String) : ConfigValue()
    data class Bool(val value: Boolean) : ConfigValue()
}

data class ConfigField(val key: String, val value: ConfigValue)

/** Order-preserving: org.json's JSONObject keeps insertion order, matching the device's response. */
fun JSONObject.toConfigFields(): List<ConfigField> {
    val fields = mutableListOf<ConfigField>()
    val keys = keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val raw = get(key)
        val value = when (raw) {
            is Boolean -> ConfigValue.Bool(raw)
            is kotlin.Number -> ConfigValue.Number(raw.toString())
            else -> ConfigValue.Text(if (raw == JSONObject.NULL) "" else raw.toString())
        }
        fields.add(ConfigField(key, value))
    }
    return fields
}

/** One modifiable property of the device's setup JSON (protocol/schema/mod_types.schema.json). */
data class Property(
    val name: String,
    val title: String,
    /** NVS key of a "key" section property, null in a "mask" section. */
    val key: String?,
    /** Bit index in the section value, "mask" section only. */
    val shift: Int,
    /** The value on the device, or the default while the device has none yet. */
    val value: ConfigValue,
    /** Shown as dots instead of text. */
    val isMasked: Boolean,
    val maxLength: Int?,
)

data class Section(
    val name: String,
    val title: String,
    /** "return": "mask" - the bits are sent back as one binary value under [key]. */
    val isMask: Boolean,
    val key: String?,
    val bits: Int,
    val properties: List<Property>,
)

private const val FORMAT_VERSION = 1

/** Parses the device's setup JSON: "format" plus one object per section, in the device's order. */
fun JSONObject.toSections(): List<Section> {
    val version = optJSONObject("format")?.optInt("ver", -1) ?: -1
    check(version == FORMAT_VERSION) { "unsupported device format version $version" }
    val sections = mutableListOf<Section>()
    val names = keys()
    while (names.hasNext()) {
        val name = names.next()
        if (name == "format") continue
        val section = getJSONObject(name)
        val properties = section.getJSONObject("properties")
        val propertyNames = properties.keys()
        val parsed = mutableListOf<Property>()
        while (propertyNames.hasNext()) {
            val propertyName = propertyNames.next()
            parsed.add(properties.getJSONObject(propertyName).toProperty(propertyName))
        }
        sections.add(
            Section(
                name = name,
                title = section.getString("title"),
                isMask = section.optString("return", "key") == "mask",
                key = if (section.has("key")) section.getString("key") else null,
                bits = section.optInt("bits", 0),
                properties = parsed,
            )
        )
    }
    return sections
}

private fun JSONObject.toProperty(name: String): Property {
    // a missing "value" means the key is not stored on the device yet: show the default
    val raw = if (has("value")) get("value") else get("default")
    return Property(
        name = name,
        title = getString("title"),
        key = if (has("key")) getString("key") else null,
        shift = optInt("shift", 0),
        value = when (raw) {
            is Boolean -> ConfigValue.Bool(raw)
            is kotlin.Number -> ConfigValue.Number(raw.toString())
            else -> ConfigValue.Text(raw.toString())
        },
        isMasked = optBoolean("mask", false),
        maxLength = optJSONArray("range")?.getInt(1),
    )
}

/**
 * Renders every value as the body for POST /api/set: a "[string]" section of key=value lines and a
 * "[binary]" section of key=value lines where the value is the little-endian hex of the combined bits.
 */
fun List<Section>.toBody(): String = buildString {
    val keySections = filter { !it.isMask }
    if (keySections.isNotEmpty()) {
        append("[string]\n")
        for (property in keySections.flatMap { it.properties }) {
            val raw = when (val value = property.value) {
                is ConfigValue.Text -> value.value
                is ConfigValue.Number -> value.value
                is ConfigValue.Bool -> value.value.toString()
            }
            append(property.key).append('=').append(raw).append('\n')
        }
    }
    val maskSections = filter { it.isMask }
    if (maskSections.isNotEmpty()) {
        append("[binary]\n")
        for (section in maskSections) {
            var mask = 0L
            for (property in section.properties) {
                if ((property.value as? ConfigValue.Bool)?.value == true) mask = mask or (1L shl property.shift)
            }
            val bytes = if (section.bits <= 16) 2 else 4
            append(section.key).append('=')
            for (i in 0 until bytes) append("%02x".format((mask ushr (8 * i)) and 0xFFL))
            append('\n')
        }
    }
}

/** Renders as an INI document, e.g. "[setup]\nkey=value\n...", for POSTing to /api/set. */
fun List<ConfigField>.toIniString(section: String = "setup"): String = buildString {
    append('[').append(section).append("]\n")
    for (field in this@toIniString) {
        val raw = when (val value = field.value) {
            is ConfigValue.Text -> value.value
            is ConfigValue.Number -> value.value
            is ConfigValue.Bool -> value.value.toString()
        }
        append(field.key).append('=').append(raw).append('\n')
    }
}
