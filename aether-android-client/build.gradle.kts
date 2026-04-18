plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "com.aethernet.client"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.aethernet.client"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.compose.ui:ui:1.5.4")
    implementation("androidx.compose.material3:material3:1.1.2")
}
