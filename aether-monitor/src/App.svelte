<script lang="ts">
  // Dashboard Logic
</script>

<main class="dashboard">
  <h1>AetherNet <span>Global Monitor</span></h1>
  <div class="metrics">
    <!-- Graph components will go here -->
    <p>Waiting for core daemon connection...</p>
  </div>
</main>

<style>
  :global(body) {
    margin: 0;
    font-family: 'Inter', sans-serif;
    background: #0f172a;
    color: #f8fafc;
  }
  
  .dashboard {
    padding: 2rem;
  }

  h1 {
    font-weight: 300;
    letter-spacing: -0.05em;
  }

  h1 span {
    font-weight: 700;
    color: #38bdf8;
  }
</style>
