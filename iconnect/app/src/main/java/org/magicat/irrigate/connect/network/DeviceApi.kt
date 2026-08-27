package org.magicat.irrigate.connect.network

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

/** Talks to a device's local setup HTTP API (GET /api/status, POST /api/set). */
object DeviceApi {
    private const val TIMEOUT_MS = 5000

    suspend fun fetchStatus(baseUrl: String): JSONObject = withContext(Dispatchers.IO) {
        val connection = (URL("$baseUrl/api/status").openConnection() as HttpURLConnection)
        try {
            connection.requestMethod = "GET"
            connection.connectTimeout = TIMEOUT_MS
            connection.readTimeout = TIMEOUT_MS
            checkSuccess(connection)
            val body = connection.inputStream.bufferedReader().use { it.readText() }
            JSONObject(body)
        } finally {
            connection.disconnect()
        }
    }

    /** [payload] is an INI document, e.g. "[setup]\nkey=value\n...". */
    suspend fun pushStatus(baseUrl: String, payload: String) = withContext(Dispatchers.IO) {
        val connection = (URL("$baseUrl/api/set").openConnection() as HttpURLConnection)
        try {
            connection.requestMethod = "POST"
            connection.connectTimeout = TIMEOUT_MS
            connection.readTimeout = TIMEOUT_MS
            connection.doOutput = true
            connection.setRequestProperty("Content-Type", "text/plain; charset=utf-8")
            connection.outputStream.use { it.write(payload.toByteArray(Charsets.UTF_8)) }
            checkSuccess(connection)
        } finally {
            connection.disconnect()
        }
    }

    suspend fun restartDevice(baseUrl: String) = withContext(Dispatchers.IO) {
        val connection = (URL("$baseUrl/api/restart").openConnection() as HttpURLConnection)
        try {
            connection.requestMethod = "POST"
            connection.connectTimeout = TIMEOUT_MS
            connection.readTimeout = TIMEOUT_MS
            connection.doOutput = true
            connection.setFixedLengthStreamingMode(0)
            checkSuccess(connection)
        } finally {
            connection.disconnect()
        }
    }

    private fun checkSuccess(connection: HttpURLConnection) {
        val code = connection.responseCode
        if (code !in 200..299) throw IOException("HTTP $code")
    }
}
